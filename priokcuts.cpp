//////////////////////////////////////////////////////////////////////////////////
//
// Company: Universidade Federal do Rio Grande do Sul
// Engineer: Rafael de Oliveira Calçada
//
// Create Date: 19.09.2019 16:12:57
// Description: Priority K-cut Algorithm Implementation
//
// Revision:
// Revision 1.0 - Improving memory performance
//
//////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <iomanip>
#include <fstream>
#include <time.h>
#include <string.h>
#include <vector>
using namespace std;

/*
 * GENERAL INFO
 ********************************************************************************/

/*	MEMORY USAGE vs. AIG SIZE

		8 * A bytes for the edges
		16 * M bytes for the vertices
		(4 + 4 * max_inputs) * max_cuts * M bytes for the cuts
		4 * M bytes for auxiliary data */

/*	ALGORITHM THEORETICAL CAPACITY

		An AIG graph of up to 1.073.741.824 vertices. For such AIG,
		the algorithm uses 24 GBytes + 4*max_cuts*max_inputs GBytes of RAM */

/*	MEMORY USAGE IN TYPICAL APPLICATIONS AND USAGE OF CACHE MEMORY

		Practical applications of this algorithm can operate on graphs
		of up to one million vertices. For these graphs, memory usage is
		approximately 24 to 240 MB, depending on the values
		of max_cuts and max_inputs.
	
		For small AIG graphs (up to 1000 vertices), graph data is expected
		to fit entirely into the L1 cache of modern processors,
		leading to extremely fast executions. */


/*
 * DATA STRUCTURES
 ********************************************************************************/

// structures that forms the AIG graph
typedef int edge;

typedef struct v {
	int i1;
	int i2;
	int fanout;
	int layer;
} vertex;

// 'lists' that describes the AIG
vertex* vertices;
edge* edges;
int* outputs;

// global variables that describes the AIG
int num_variables = 0;
int num_inputs = 0;
int num_latches = 0;
int num_outputs = 0;
int num_ands = 0;

// other globals
int cut_offset = 0;
int max_cuts = 0;
int max_inputs = 0;


// 'lists' used by the algorithm to evaluate the results
float* cut_costs;
int* cut_inputs;
vector<vector<int>*>* layers;
vector<int>* next_layer;
vector<int>* current_layer;
vector<int>* preceding_vertices;


/*
 * HELPFUL FUNCTIONS AND PROCEDURES
 ********************************************************************************/


// this procedure creates the graph used by the main function
// opens, read and process the input file, allocating memory efficiently
void create_graph_from_input_file(char* filename)
{

	// opens the input file
	ifstream input_file;
	input_file.open(filename, ifstream::in);

	if(!input_file.is_open())
	{
		cerr << "Failed to open the input file." << endl;
		exit(-1);
	}

	// process the 1st line
	char buffer[256];
	buffer[0] = '\0';
	input_file.getline(buffer, sizeof(buffer));

	// file format check
	if(strlen(buffer) > 2 && buffer[0] != 'a' && buffer[1] != 'a' && buffer[2] != 'g')
	{
		cerr << "Failed to process the input file. Wrong, invalid or unknown format." << endl;
		exit(-1);
	}

	// split into tokens, saving the values in variables
	char* token = strtok(buffer," ");
	token = strtok(NULL, " ");
	num_variables = atoi(token);
	token = strtok(NULL, " ");
	num_inputs = atoi(token);
	token = strtok(NULL, " ");
	num_latches = atoi(token);
	token = strtok(NULL, " ");
	num_outputs = atoi(token);
	token = strtok(NULL, " ");
	num_ands = atoi(token);

	// check for latches
	if(num_latches != 0)
	{
		cerr << "This graph contains latches. The current version of this implementation do not support them." << endl;
		exit(-1);
	}

	// integrity check #1
	if(num_variables != num_inputs + num_latches + num_ands)
	{
		cerr << "This graph is invalid. M != I + L + A." << endl;
		exit(-1);
	}

	// OK. Now we're ready for memory allocation
	outputs = new int[num_outputs];
	vertices = new vertex[num_variables];
	edges = new edge[(num_ands << 1)];

	// initialization of output list
	for(int i = 0; i < num_outputs; i++) outputs[i] = -1;

	// creates the input vertices
	for(int i = 0; i < num_inputs; i++)
	{
		input_file.getline(buffer, sizeof(buffer));
		if(strlen(buffer) < 1)
		{
			cerr << "The input file reached the end before expected." << endl;
			exit(-1);
		}
		token = strtok(buffer, " ");
		int label = atoi(token);

		// integrity check #2
		if(label < 0)
		{
			cerr << "The graph contains an invalid (negative) input index: " << label << "." << endl;
			exit(-1);
		}

		// integrity check #4
		if(label != ((i+1)*2))
		{
			cerr << "The AIG format states that the label of an input must be twice its index, but the input with index " << i+1 << " has the label " << label << "." << endl;
			exit(-1);
		}

		// if reached here, everything is OK, so set the value of the incoming edges
		// to -1 (indicating no incoming edges), fanout to 0 and layer number to 1
		vertices[i].i1 = -1;
		vertices[i].i2 = -1;
		vertices[i].fanout = 0;
		vertices[i].layer = 1;
	}

	// save the label of the output vertices
	for(int i = 0; i < num_outputs; i++)
	{
		input_file.getline(buffer, sizeof(buffer));
		if(strlen(buffer) < 1)
		{
			cerr << "The input file reached the end before expected." << endl;
			exit(-1);
		}
		token = strtok(buffer, " ");
		int label = atoi(token);

		// integrity check #5
		if(label < 0)
		{
			cerr << "The graph contains an invalid (negative) output index: " << label << "." << endl;
			exit(-1);
		}

		// integrity check #6
		for(int j = 0; j < num_outputs; j++)
			if(outputs[j] == label)
			{
				cerr << "The graph contains an output declared twice: " << label << "." << endl;
				exit(-1);
			}

		// if reached here, everything is OK, so adds the label in the outputs list
		outputs[i] = label;
	}

	// creates the vertices and its edges
	for(int i = 0; i < num_ands; i++)
	{
		input_file.getline(buffer, sizeof(buffer));
		if(strlen(buffer) < 1)
		{
			cerr << "The input file reached the end before expected." << endl;
			exit(-1);
		}
		token = strtok(buffer, " ");
		int label = atoi(token);

		// integrity check #7
		if(label < 0)
		{
			cerr << "The graph contains an invalid (negative) vertex index: " << label << "." << endl;
			exit(-1);
		}

		// integrity check #8
		if(label != ((i+num_inputs+1)*2))
		{
			cerr << "The AIG format states that the label of a vertex must be twice its index, but the vertex with index " << i+1 << " has the label " << label << "." << endl;
			exit(-1);
		}
		token = strtok(NULL, " ");
		int i1 = atoi(token);
		token = strtok(NULL, " ");
		int i2 = atoi(token);

		// integrity check #9
		if(i1 < i2)
		{
			cerr << "The AIG format states that the label of the first input of a vertex must be greater than the second." << endl;
			cerr << "Found i1=" << i1 << " and i2=" << i2 << " for the label " << label << "." << endl;
			exit(-1);			
		}

		// integrity check #10
		if(i1 < 2 || i2 < 2)
		{
			cerr << "The vertex has an invalid value for its inputs." << endl;
			cerr << "Found i1=" << i1 << " and i2=" << i2 << " for the label " << label << "." << endl;
			exit(-1);			
		}

		// integrity check #11
		if(label <= i1 || label <= i2)
		{
			cerr << "The AIG format states that the label must be greater than the value of its inputs." << endl;
			cerr << "Found i1=" << i1 << " and i2=" << i2 << " for the label " << label << "." << endl;
			exit(-1);			
		}

		// if reached here, everything is OK, so adds the vertex into in the list, creates its edges, and updates the fanout of the child vertices
		vertices[i+num_inputs].i1 = i1;
		vertices[i+num_inputs].i2 = i2;
		vertices[i+num_inputs].fanout = 0;
		edges[i<<1] = i1 / 2 - 1;
		edges[(i<<1)+1] = i2 / 2 - 1;
		int iv1 = i1 >> 1;
		int iv2 = i2 >> 1;
		vertices[iv1-1].fanout += 1;
		vertices[iv2-1].fanout += 1;
		
		// evaluate and saves the vertex layer number
		int layer_i1 = vertices[iv1-1].layer;
		int layer_i2 = vertices[iv2-1].layer;
		int max_layer = layer_i1 > layer_i2 ? layer_i1 : layer_i2;
		vertices[i+num_inputs].layer = max_layer + 1;
		/*
		// if the list of vertices of the layer number N do not exists, creates and add the vertex into it
		// if exists, just add the vertex into it
		if(layers->size() < max_layer)
		{
			vector<int>* layer = new vector<int>;
			layers->push_back(layer);
			layer->push_back(i+num_inputs);
		}
		else
		{
			vector<int>* layer = layers->at(max_layer);
			layer->push_back(i+num_inputs);
		}*/

	}
	
	// updates the fanout of the output vertices
	for(int i = 0; i < num_outputs; i++)
	{
		int output_index = outputs[i] >> 1;
		vertices[output_index-1].fanout += 1;
	}

	// integrity check #12
	for(int i = 0; i < num_variables; i++)
	{
		if(vertices[i].fanout <= 0)
		{
			cerr << "There is a vertex (" << (i+1)*2 << ") in the graph that has no outcoming edge (fanout = 0)." << endl;
			exit(-1);
		}
	}

	for(int i = 0; i < num_variables; i++)
	{
		cout << i << " - layer " << vertices[i].layer << endl;
	}

}

// get an iterator to some element in a vector
vector<int>::iterator get_iterator(vector<int>* vec, int elem)
{
	for(vector<int>::iterator it = vec->begin(); it != vec->end(); ++it)
	{
		if((*it) == elem)
		{
			return it;
		}
	}
	return vec->end();
}

// checks if a product of cuts matches with a cut in the list
bool match_with_a_cut_in_the_list(vector<int>* product, int vertex_index)
{
	bool match = false;
	for(int i = 0; i < max_cuts; i++)
	{
		int num_inputs_found = 0;
		for(int j = 0; j < product->size(); j++)
		{
			bool input_found = false;
			for(int k = 0; k < max_inputs; k++)
			{
				if(cut_inputs[vertex_index*cut_offset+i*max_inputs+k] == product->at(j))
				{
					input_found = true;
				}
			}
			if(input_found) num_inputs_found += 1;
		}
		if(num_inputs_found == product->size()) return true;
	}
	return false;
}

// return the index of the winner cut
int winner_cut(int vertex_index)
{
	int winner_cost = cut_costs[vertex_index*max_cuts];
	int winner_index = 0;
	for(int j = 0; j < max_cuts; j++)
	{
		if(cut_costs[vertex_index*max_cuts + j] < 0) continue;
		if(winner_cost >= cut_costs[vertex_index*max_cuts + j] ||
		cut_costs[vertex_index*max_cuts + j] > 0 && winner_cost < 0)
		{
			winner_cost = cut_costs[vertex_index*max_cuts + j];
			winner_index = j;
		}
	}
	return winner_index;
}

// return the index of the loser cut
int loser_cut(int vertex_index)
{
	int loser_cost = 0;
	int loser_index = 0;
	for(int j = 0; j < max_cuts; j++)
	{
		if(loser_cost <= cut_costs[vertex_index*max_cuts + j])
		{
			loser_cost = cut_costs[vertex_index*max_cuts + j];
			loser_index = j;
		}
	}
	return loser_index;
}

// check if a vertex is in the list
bool in_the_list(int vertex_index, vector<int>* list)
{
	for(int i = 0; i < list->size(); i++)
		if(list->at(i) == vertex_index) 
			return true;
	return false;
}

// prints the cuts of a vertex on screen
void print_cuts(int vertex_index)
{
	int vertex_label = (vertex_index+1) << 1;
	cout << "  v(" << vertex_label << ") has cuts:" << endl;
	for(int i = 0; i < max_cuts; i++)
	{
		if(cut_costs[vertex_index*max_cuts+i] != -1) {
			cout << "    { ";
			for(int j = 0; j < max_inputs; j++)
				if(cut_inputs[vertex_index*cut_offset+i*max_inputs+j] != -1)
					cout << cut_inputs[vertex_index*cut_offset+i*max_inputs+j] << " ";
			cout << "} with cost " << cut_costs[vertex_index*max_cuts+i] << endl;
		}
	}
}


/*
 * MAIN FUNCTION: COMPUTES THE PRIORITY K-CUTS FOR THE AIG
 ********************************************************************************/


int main(int argc, char* argv[])
{

	max_cuts = 2;
	max_inputs = 3;

	double time_spent = 0.0;
	clock_t start = clock();

	// check for correct usage
	if(argc != 2 && argc != 4)
	{
		cerr << endl << "  Usages: " << endl << endl;
		cerr << "  " << argv[0] << " [input-file]" << endl;
		cerr << "  " << argv[0] << " [input-file] [max-cuts] [max-inputs]" << endl << endl;
		cerr << "  [input-file] = an AIG file in the ASCII format." << endl;
		cerr << "  [max-cuts] = the maximum number of cuts stored for each vertex in the AIG." << endl;
		cerr << "  [max-inputs] = the maximum number of inputs for a cut." << endl << endl;
		cerr << "  If not provided, the value of [max-cuts] and [max-inputs] "
			 << "is set to 2 and 3, respectively." << endl << endl;
		return -1;
	}

	if(argc == 4)
	{
		max_cuts = atoi(argv[2]);
		max_inputs = atoi(argv[3]);
	}

	if(max_cuts < 2 || max_inputs < 2)
	{
		cerr << "Minimal value for [max-cuts] and [max-inputs] is 2." << endl;
		exit(-1);
	}

	// creates the layers list
	layers = new vector<vector<int>*>;

	// creates the AIG graph
	create_graph_from_input_file(argv[1]);

	/*
	 * ABOUT THE ALGORITHM
	 *
	 * A "layer" of vertices is a set of vertices witch edges comes only from preceding layers.
	 * The first layer is the set of inputs. The second layer has edges that comes from one of the input vertices.
	 * The third layer has edges that comes from the first or second layer, and so on...
	 * The algorithm evaluates the set of vertices of a layer based on 'current_layer'.
	 * The algorithm begins making the input set the 'current_layer'.
	 * Then, it evaluates the set of vertices of the next layer based on current_layer (and calls this set 'next_layer').
	 * Then, it evaluates the priority k-cuts. Then and sets current_layer = next_layer.
	 * The algorithm stops when, after evaluating next_layer, the result is an empty set.
	 *
	 */

	// first of all, allocate memory for the cuts
	cut_offset = max_cuts*max_inputs;
	cut_costs = new float[num_variables*max_cuts];
	cut_inputs = new int[num_variables*cut_offset];

	// set to zero the cost of the input vertex cut
	// the other spaces in the vector are initialized with -1
	for(int i = 0; i < num_inputs; i++)
	{
		cut_costs[(i*max_cuts)] = 0;
		cut_inputs[i*cut_offset] = (i+1) << 1;
		for(int k = 1; k < max_inputs; k++)	cut_inputs[i*cut_offset+k] = -1;
		for(int j = 1; j < max_cuts; j++)
		{
			cut_costs[(i*max_cuts) + j] = -1;
			for(int k = 0; k < max_inputs; k++)	cut_inputs[i*cut_offset+j*max_inputs+k] = -1;
		}
	}

	// make current_layer the input set and add the inputs in the preceding_vertices list
	current_layer = new vector<int>;
	preceding_vertices = new vector<int>;
	for(int i = 0; i < num_inputs; i++) 
	{
		current_layer->push_back(i);
		preceding_vertices->push_back(i);
	} 

	int layer_number = 1;

	cout << "Input set (layer n. " << layer_number << "):" << endl;
	for(int i = 0; i < num_inputs; i++) print_cuts(i);
	layer_number++;

	int next_layer_size = 0;
	vector<int>* previous_layer = NULL;
	do
	{
		// First, initialize next_layer as an empty set and deallocates the previous later if it exists
		next_layer = new vector<int>;
		if(previous_layer != NULL) delete previous_layer;

		// Evaluates the number of edges that leaves the current_layer set.
		// The search in the edges list will stop when 'num_edges' were found,
		// witch turns the execution a lot faster
		int num_edges = 0;
		for(int i = 0; i < current_layer->size(); i++) num_edges += vertices[current_layer->at(i)].fanout;

		// Then, add in the next_layer all the vertices pointed by the edges
		// that leaves a vertex from the current_layer.
		int num_edges_found = 0;
		for(int i = 0; i < (num_ands << 1); i+=2)
		{
			for(int j = 0; j < current_layer->size(); j++)
			{
				int vertex_src_index = current_layer->at(j);
				int vertex_dst_index = (i / 2) + num_inputs;
				if(edges[i] == vertex_src_index)
				{
					num_edges_found += 1;
					if(!in_the_list(vertex_dst_index,next_layer))
						next_layer->push_back(vertex_dst_index);
				}
				if(edges[i+1] == vertex_src_index)
				{
					num_edges_found += 1;
					if(!in_the_list(vertex_dst_index,next_layer))
						next_layer->push_back(vertex_dst_index);
				}
			}
			if(num_edges == num_edges_found) break;
		}

		/*
		 * At this point, next_layer has all the vertices pointed by the edges that leaves the vertices in the current_layer set,
		 * BUT, note that these vertices can be pointed also by any other vertices, including vertices that are not in the
		 * current_layer set or any preceding layer.
		 *
		 * To fix this, there is a list with only the vertices that are in the current_layer or in a preceding layer,
		 * called 'preceding_vertices'
		 *
		 * SO, we need to check the edges that comes in to a vertex currently in the next_layer set. If the source of an edge
		 * is not a vertex of preceding_vertices, we need to remove the vertex pointed by this edge from next_layer
		 *
		 */

		// finds the vertices that must not be in the next_list and put them in the remove_list
		vector<int>* remove_list = new vector<int>;
		for(int i = 0; i < next_layer->size(); i++)
		{
			int edges_index = (next_layer->at(i)+1) * 2;
			int edge1_src = edges[(next_layer->at(i)-num_inputs)*2];
			int edge2_src = edges[(next_layer->at(i)-num_inputs)*2+1];
			if(!in_the_list(edge1_src,preceding_vertices))
				remove_list->push_back(next_layer->at(i));
			if(!in_the_list(edge2_src,preceding_vertices))
				remove_list->push_back(next_layer->at(i));
		}

		// then remove
		for(int i = 0; i < remove_list->size(); i++)
		{
			vector<int>::iterator it = get_iterator(next_layer, remove_list->at(i));
			if(it == next_layer->end())
			{
				cerr << "Fail to remove a vertex from the next_layer set. This should not occur." << endl;
				exit(-1);
			}
			else next_layer->erase(it);
		}

		/*
 		 * EVALUATE THE PRIORITY K-CUTS
		 ************************************************************************/

		// Makes the cartesian product of the cuts of the vertex leaves
		for(int i = 0; i < next_layer->size(); i++)
		{	
			
			// initialize the cuts of the vertex
			int vertex_index = next_layer->at(i);
			for(int j = 0; j < max_cuts; j++)
			{
				cut_costs[vertex_index*max_cuts + j] = -1.0;
				for(int k = 0; k < max_inputs; k++)
					cut_inputs[vertex_index*cut_offset+j*max_inputs+k] = -1;
			}
			
			// cartesian product
			int leaf1_vertex_index = edges[(vertex_index-num_inputs)*2];
			int leaf2_vertex_index = edges[(vertex_index-num_inputs)*2+1];
			vector<int> product;
			vector<int> cut1_inputs;
			vector<int> cut2_inputs;
			float product_cost;
			float cut1_cost;
			float cut2_cost;

			for(int j = 0; j < max_cuts; j++)
				for(int k = 0; k < max_cuts; k++)
				{

					// erases previous content
					product.clear();
					cut1_inputs.clear();
					cut2_inputs.clear();

					// get the cost of the cuts
					cut1_cost = cut_costs[leaf1_vertex_index*max_cuts + j];
					cut2_cost = cut_costs[leaf2_vertex_index*max_cuts + k];
					product_cost = cut1_cost + cut2_cost;

					// if the cuts cost is not negative, make the product
					if(cut1_cost < 0 || cut2_cost < 0) continue;
					else 
					{
						// get the inputs of the cuts
						for(int l = 0; l < max_inputs; l++)
							cut1_inputs.push_back(cut_inputs[leaf1_vertex_index*cut_offset+j*max_inputs+l]);
						for(int l = 0; l < max_inputs; l++)
							cut2_inputs.push_back(cut_inputs[leaf2_vertex_index*cut_offset+k*max_inputs+l]);
						for(int l = 0; l < cut1_inputs.size(); l++)
							if(cut1_inputs.at(l) != -1)
								product.push_back(cut1_inputs.at(l));
						for(int l = 0; l < cut2_inputs.size(); l++)
							if(cut2_inputs.at(l) != -1)
								if(!in_the_list(cut2_inputs.at(l), &product))
									product.push_back(cut2_inputs.at(l));
						
						// verifies the cost of the cuts
						// if the product has a lower cost, replaces the cut
						// if the product has more than max_input inputs, do nothing
						// only inserts the cut if it does not match any cut already in the list
						if(product.size() <= max_inputs && !match_with_a_cut_in_the_list(&product,vertex_index))
						{
							bool inserted = false;
							for(int l = 0; l < max_cuts; l++)
							{
								if(!inserted) {
									int actual_cost = cut_costs[vertex_index*max_cuts + l];
									if(actual_cost == -1 || actual_cost > product_cost)
									{
										cut_costs[vertex_index*max_cuts + l] = product_cost;
										for(int m = 0; m < max_inputs; m++)
											cut_inputs[vertex_index*cut_offset+l*max_inputs+m] = -1;
										for(int m = 0; m < product.size(); m++)
											cut_inputs[vertex_index*cut_offset+l*max_inputs+m] = product.at(m);
										inserted = true;
									}
								}
							} 
						}
					}					
				}

			// computes the winner cut and evaluates the cost of the autocut
			int winner = winner_cut(vertex_index);
			float winner_cost = cut_costs[vertex_index*max_cuts + winner];
			float autocut_cost = winner_cost + (1.0 / (float) vertices[vertex_index].fanout);

			// inserts the autocut in a free position (if there is), OR
			// replaces a cut "worse" than the autocut (a cut with higher cost)
			bool replaced_or_inserted = false;
			for(int l = 0; l < max_cuts; l++)
				if(!replaced_or_inserted) {
					int actual_cost = cut_costs[vertex_index*max_cuts + l];
					if(actual_cost == -1 || actual_cost > autocut_cost)
					{
							cut_costs[vertex_index*max_cuts + l] = autocut_cost;
							for(int m = 0; m < max_inputs; m++)
								cut_inputs[vertex_index*cut_offset+l*max_inputs+m] = -1;
							cut_inputs[vertex_index*cut_offset+l*max_inputs] = (vertex_index+1)*2;
							replaced_or_inserted = true;
					}
				}

			// if there's no free position and no cut is worse than the autocut,
			// chooses the cut with the highest cost ("loser" cut) and replaces it
			if(!replaced_or_inserted)
			{				
				int loser_cut_index = loser_cut(vertex_index);
				cut_costs[vertex_index*max_cuts + loser_cut_index] = autocut_cost;
				for(int m = 0; m < max_inputs; m++)
					cut_inputs[vertex_index*cut_offset+loser_cut_index*max_inputs+m] = -1;
				cut_inputs[vertex_index*cut_offset+loser_cut_index*max_inputs] = (vertex_index+1)*2;
			}
		}

		// prints the results on screen
		if(next_layer->size() > 0)
		{
			cout << endl << "Layer n. " << layer_number << ":" << endl;
			for(int i = 0; i < next_layer->size(); i++) print_cuts(next_layer->at(i));
		}
		layer_number++;

		// prepare the lists for the next iteration
		previous_layer = current_layer;
		current_layer = next_layer;
		for(int i = 0; i < next_layer->size(); i++) preceding_vertices->push_back(next_layer->at(i));
		next_layer_size = next_layer->size();

	} while (next_layer_size > 0);

	// evaluates the execution time
	clock_t end = clock();
	time_spent += ((double)(end - start) / CLOCKS_PER_SEC);
	cout.setf(std::ios::fixed);
	cout.precision(2);
	cout << endl << "Execution time (sec): " << time_spent << endl;
	cout << "Execution time (ms):  " << time_spent * 1000.0 << endl;
	cout << "Execution time (us):  " << time_spent * 1000000.0 << endl << endl;

	return 0;

}
