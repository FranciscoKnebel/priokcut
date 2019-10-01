//////////////////////////////////////////////////////////////////////////////////
//
// Company: Universidade Federal do Rio Grande do Sul
// Engineer: Rafael de Oliveira Calçada
//
// Create Date: 19.09.2019 16:12:57
// Description: Priority K-cut Algorithm Implementation
//
// Revision:
// Revision 2.0 - Performance enhancement
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

/*	MEMORY USAGE vs. AIG SIZE and other parameters
	
		Let M be the number of vertices, I the maximum number
		of inputs allowed in the cuts and K the maximum number of cuts for each
		vertex. So, the algorithm will use:

		16 * M bytes to store the vertices
		(4 + 4 * I) * K * M bytes for the cuts
		4 * M bytes for auxiliary data */

/*	ALGORITHM THEORETICAL CAPACITY

		An AIG graph of up to 1.073.741.824 vertices. For such AIG,
		the algorithm uses 24 GB + 4 * K * I GB of RAM */

/*	MEMORY USAGE IN TYPICAL APPLICATIONS AND USAGE OF CACHE MEMORY

		The algorithm was tested for very large graphs. In an 8GB RAM
		Intel Core-i7 machine, the algorithm takes about 1 second to
		process 1.6 million of vertices with k = 2 and i = 6.
		In general, the lower the values of i and k, the lower
		the memory usage and execution time.
	
		For small AIG graphs (up to 10000 vertices), graph data is expected
		to fit entirely into the L1 cache of modern processors,
		leading to extremely fast executions. */


/*
 * DATA STRUCTURES
 ********************************************************************************/

// structures that forms the AIG graph

typedef struct v {
	int i1;
	int i2;
	int fanout;
	int layer;
} vertex;

// 'lists' that describes the AIG
vertex* vertices;
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
bool display = false;
char* filename = NULL;


// 'lists' used by the algorithm to evaluate the results
float* cut_costs;
int* cut_inputs;
vector<vector<int>*>* layers;
vector<int>* next_layer;


/*
 * HELPFUL FUNCTIONS AND PROCEDURES
 ********************************************************************************/

// get a char from a file in the AIGER binary format
unsigned char getnoneofch(ifstream& input_file)
{
	int ch = input_file.get();
	if(ch != EOF) return ch;
	cerr << "*** decode: unexpected EOF" << endl;
	exit(-1);
}

// decodes a delta encoding from a file in the AIGER binary format
unsigned int decode(ifstream& input_file)
{
	unsigned x = 0, i = 0;
	unsigned char ch;
	while ((ch = getnoneofch(input_file)) & 0x80)
	{
		x |= (ch & 0x7f) << (7 * i++);
	}
	return x | (ch << (7 * i));
}

// process the file in the ASCII format
void process_ascii_format(ifstream& input_file)
{
	char* token;
	char buffer[256];
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
		if(i1 < 0 || i2 < 0)
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
		vertices[i+num_inputs].fanout = 0;
		if(i1 >= 2) vertices[i+num_inputs].i1 = i1 / 2 - 1;
		else vertices[i+num_inputs].i1 = -i1-2;
		if(i2 >= 2)	vertices[i+num_inputs].i2 = i2 / 2 - 1;
		else vertices[i+num_inputs].i2 = -i2-2;
		int iv1 = i1 >> 1;
		int iv2 = i2 >> 1;
		if(i1 >= 2) vertices[iv1-1].fanout += 1;
		if(i2 >= 2) vertices[iv2-1].fanout += 1;
		
		// evaluate and saves the vertex layer number
		int layer_i1 = 1;
		if(i1 >= 2) layer_i1 = vertices[iv1-1].layer;
		int layer_i2 = 1;
		if(i2 >= 2) layer_i2 = vertices[iv2-1].layer;
		int max_layer = layer_i1 > layer_i2 ? layer_i1 : layer_i2;
		vertices[i+num_inputs].layer = max_layer + 1;
		
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
			vector<int>* layer = layers->at(max_layer-1);
			layer->push_back(i+num_inputs);
		}		

	}
	
	// updates the fanout of the output vertices
	for(int i = 0; i < num_outputs; i++)
	{
		int output_index = outputs[i] >> 1;
		vertices[output_index-1].fanout += 1;
	}

}

// process the file in the binary format
void process_binary_format(ifstream& input_file)
{
	char* token;
	char buffer[256];

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

	// initialization of output list
	for(int i = 0; i < num_outputs; i++) outputs[i] = -1;

	// creates the input vertices
	for(int i = 0; i < num_inputs; i++)
	{
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
		
		unsigned int delta0 = decode(input_file);
		unsigned int delta1 = decode(input_file);

		int label = (num_inputs+i+1)*2;
		int i1 = label - delta0;
		int i2 = i1 - delta1;

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

		// integrity check #9
		if(i1 < i2)
		{
			cerr << "The AIG format states that the label of the first input of a vertex must be greater than the second." << endl;
			cerr << "Found i1=" << i1 << " and i2=" << i2 << " for the label " << label << "." << endl;
			exit(-1);			
		}

		// integrity check #10
		if(i1 < 0 || i2 < 0)
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
		vertices[i+num_inputs].fanout = 0;
		if(i1 >= 2) vertices[i+num_inputs].i1 = i1 / 2 - 1;
		else vertices[i+num_inputs].i1 = -i1-2;
		if(i2 >= 2)	vertices[i+num_inputs].i2 = i2 / 2 - 1;
		else vertices[i+num_inputs].i2 = -i2-2;
		int iv1 = i1 >> 1;
		int iv2 = i2 >> 1;
		if(i1 >= 2) vertices[iv1-1].fanout += 1;
		if(i2 >= 2) vertices[iv2-1].fanout += 1;
		
		// evaluate and saves the vertex layer number
		int layer_i1 = 1;
		if(i1 >= 2) layer_i1 = vertices[iv1-1].layer;
		int layer_i2 = 1;
		if(i2 >= 2) layer_i2 = vertices[iv2-1].layer;
		int max_layer = layer_i1 > layer_i2 ? layer_i1 : layer_i2;
		vertices[i+num_inputs].layer = max_layer + 1;
		
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
			vector<int>* layer = layers->at(max_layer-1);
			layer->push_back(i+num_inputs);
		}		

	}
	
	// updates the fanout of the output vertices
	for(int i = 0; i < num_outputs; i++)
	{
		int output_index = outputs[i] >> 1;
		vertices[output_index-1].fanout += 1;
	}
}

// creates the graph used by the main function and
// split the graph in layers of vertices
void create_graph_and_split_in_layers(char* filename)
{

	// opens the input file
	ifstream input_file;
	input_file.open(filename, ios::binary | ios::in);

	if(!input_file.is_open())
	{
		cerr << "Failed to open the input file." << endl;
		exit(-1);
	}

	// process the 1st line
	char buffer[256];
	buffer[0] = '\0';
	input_file.getline(buffer, sizeof(buffer));

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

	// file format check
	if(strlen(buffer) > 2 && buffer[0] == 'a' && buffer[1] == 'a' && buffer[2] == 'g')
	{
		cout << endl <<  "Processing AIG in the ASCII format..." << endl;
		cout << "M I L O A = " << num_variables << " " << num_inputs << " " << num_latches
			 << " " << num_outputs << " " << num_ands << endl;
		process_ascii_format(input_file);
	}
	else if(strlen(buffer) > 2 && buffer[0] == 'a' && buffer[1] == 'i' && buffer[2] == 'g')
	{
		cout << endl << "Processing AIG in the binary format..." << endl;
		cout << "M I L O A = " << num_variables << " " << num_inputs << " " << num_latches
			 << " " << num_outputs << " " << num_ands << endl;
		process_binary_format(input_file);
	}
	else {
		cerr << "Failed to process the input file. Wrong, invalid or unknown format." << endl;
		exit(-1);
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

// show the help on screen
void show_help(char* argv[])
{
		cerr << endl << "  \e[1mUsage:\e[0m " << argv[0] << " <file> [options]" << endl << endl;
		cerr << "  <file>         An AIG in the ASCII format. This argument is required." << endl << endl;
		cerr << "  \e[1mOptions\e[0m:" << endl << endl;
		cerr << "  -i <value>     The maximum number of inputs for each cut." << endl;
		cerr << "  -k <value>     The number of cuts stored for each vertex." << endl;
		cerr << "  -d             Display the results on the screen (may slow down the execution time for large graphs)." << endl << endl;
		cerr << "  -h --help      This help." << endl << endl;
		cerr << "  If not provided, the values of i and k are set to 2 and 3, respectively." << endl << endl;
}

// process the arguments passed by command line interface
void process_args(int argc, char* argv[])
{

	int i = 1;	
	while(i < argc)
	{
		char* arg = argv[i];
		if(arg[0] == '-')
		{
			if(strlen(arg) < 2)
			{
				cerr << "FAIL. Unknown option." << endl;
				exit(-1);					
			}
			if(arg[1] == 'd')
			{
				display = true;
				i++;
			}
			if(arg[1] == 'h')
			{
				show_help(argv);
				exit(-1);
			}
			if(arg[1] == '-')
			{
				if(strlen(arg) < 6)
				{
					cerr << "FAIL. Unknown option." << endl;
					exit(-1);					
				}
				if(arg[2] == 'h' && arg[3] == 'e' && arg[4] == 'l' && arg[5] == 'p')
				{
					show_help(argv);
					exit(-1);
				}
				else
				{
					cerr << "FAIL. Unknown option." << endl;
					exit(-1);					
				}
			}
			else if(arg[1] == 'i' || arg[1] == 'k')
			{
				if(i+1 < argc)
				{
					char* nextarg = argv[i+1];
					if(nextarg[0] == '-')
					{
						cerr << "FAIL. Missing or wrong value for -" << arg[1] << " option." << endl;
						exit(-1);						
					}
					if(arg[1] == 'i') max_inputs = atoi(nextarg);
					if(arg[1] == 'k') max_cuts = atoi(nextarg);
					i += 2;
				}
				else
				{
					cerr << "FAIL. Missing or wrong value for -" << arg[1] << " option." << endl;
					exit(-1);
				}
			}
		}
		else
		{
			filename = argv[i];
			i++;
		}
	}

	if(filename == NULL)
	{
		cerr << "FAIL. <file> parameter not provided." << endl;
		exit(-1);
	}

	if(max_cuts < 2 || max_inputs < 2)
	{
		cerr << "FAIL. Minimal value for -i and -k is 2." << endl;
		exit(-1);
	}

}


/*
 * MAIN FUNCTION: COMPUTES THE PRIORITY K-CUTS FOR THE AIG
 ********************************************************************************/


int main(int argc, char* argv[])
{

	// set the parameters default values
	max_cuts = 2;
	max_inputs = 3;
	display = false;

	// initializes the runtime count
	double time_spent = 0.0;
	clock_t start = clock();

	// check for correct usage
	if(argc < 2)
	{
		show_help(argv);
		exit(-1);
	}

	// arguments processing
	process_args(argc, argv);

	// creates the layers list
	layers = new vector<vector<int>*>;

	// creates the AIG graph
	create_graph_and_split_in_layers(argv[1]);

	/*
	 * ABOUT THE ALGORITHM
	 *
	 * A "layer" of vertices is a set of vertices witch edges comes only from preceding layers.
	 * The first layer is the set of inputs. The second layer has edges that comes from one of the input vertices.
	 * The third layer has edges that comes from the first or second layer, and so on...
	 * The algorithm evaluates the set of vertices of a layer during the processing of the input file.
	 * The algorithm begins seting the cost of the input vertex cuts to 0.
	 * Then, it evaluates the cuts and the costs of each vertex in each layer (the bottom layers are processed first).
	 *
	 */

	// first of all, allocate memory for the cuts
	cut_offset = max_cuts*max_inputs;
	cut_costs = new float[num_variables*max_cuts];
	cut_inputs = new int[num_variables*cut_offset];

	// set to zero the cost of the cut of each input vertex
	// fill the blank spaces left in the vector with -1
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

	int layer_number = 1;

	if(display)
	{
		cout << "Input set (layer n. " << layer_number << "):" << endl;
		for(int i = 0; i < num_inputs; i++) print_cuts(i);
	}
	layer_number++;

	for(int z = 0; z < layers->size(); z++)
	{

		next_layer = layers->at(z);

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
			int leaf1_vertex_index = vertices[vertex_index].i1;
			int leaf2_vertex_index = vertices[vertex_index].i2;
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
					if(leaf1_vertex_index >= 0) cut1_cost = cut_costs[leaf1_vertex_index*max_cuts + j];
					else cut1_cost = 0.0;
					if(leaf2_vertex_index >= 0) cut2_cost = cut_costs[leaf2_vertex_index*max_cuts + k];
					else cut2_cost = 0.0;
					if(vertices[vertex_index].fanout == 0)
					{
						cerr << "Found a vertex (" << (vertex_index+1)*2 << ") with fanout = 0." << endl;
						exit(-1);
					}
					else product_cost = (cut1_cost + cut2_cost) / (float) vertices[vertex_index].fanout;

					// if the cuts cost is not negative, make the product
					if(cut1_cost < 0 || cut2_cost < 0) continue;
					else 
					{
						// get the inputs of the cuts
						if(leaf1_vertex_index >= 0)
						{
							for(int l = 0; l < max_inputs; l++)
								cut1_inputs.push_back(cut_inputs[leaf1_vertex_index*cut_offset+j*max_inputs+l]);
						}
						else cut1_inputs.push_back(-(leaf1_vertex_index+2));
						if(leaf2_vertex_index >= 0)
						{
							for(int l = 0; l < max_inputs; l++)
								cut2_inputs.push_back(cut_inputs[leaf2_vertex_index*cut_offset+k*max_inputs+l]);
						}
						else cut2_inputs.push_back(-(leaf2_vertex_index+2));
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
		if(display)
		{
			cout << endl << "Layer n. " << layer_number << ":" << endl;
			for(int i = 0; i < next_layer->size(); i++) print_cuts(next_layer->at(i));
		}
		layer_number++;
	}

	// evaluates the execution time
	clock_t end = clock();
	time_spent += ((double)(end - start) / CLOCKS_PER_SEC);
	cout.setf(std::ios::fixed);
	string time_sec = to_string(time_spent);
	string time_msec = to_string((time_spent*1000.0));
	string time_usec = to_string((time_spent*1000000.0));
	time_sec.erase(time_sec.find_last_not_of('0'), string::npos);
	time_msec.erase(time_msec.find_last_not_of('0'), string::npos);
	time_usec.erase(time_usec.find_last_not_of('0'), string::npos);
	cout << endl << "Execution time (sec): " << time_sec << " s" << endl;
	cout << "Execution time (ms):  " << time_msec << " ms" << endl;
	cout << "Execution time (us):  " << time_usec << " us" << endl << endl;

	return 0;

}
