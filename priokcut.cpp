//////////////////////////////////////////////////////////////////////////////////
// Company: Universidade Federal do Rio Grande do Sul
// Engineer: Rafael de Oliveira Calçada
// 
// Create Date: 12.09.2019 16:12:57
// Description: Priority K-cut Algorithm Implementation
// 
// Revision:
// Revision 0.01 - File Created
// 
//////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <list>
using namespace std;

/*
 * DATA STRUCTURES USED BY THE ALGORITHM
 ****************************************/

// structures that forms the AIG graph
typedef struct e {
	int label;
	void* src;
	void* dst;
} edge;

typedef struct v {
	int label;
	list<edge*>* out_edges;
	list<edge*>* in_edges;
} vertex;

// these lists describes the AIG
list<edge*> edges;
list<vertex*> inputs;
list<vertex*> vertices;

// these lists are used by the algorithm to evaluate the results
list<vertex*>* next_layer;
list<vertex*>* current_layer;
list<vertex*>* preceding_vertices;


/*
 * RELATED FUNCTIONS AND PROCEDURES
 ***********************************/

// check for some vertex in a list
bool vertex_exists(list<vertex*>* list, int label)
{
	for(std::list<vertex*>::iterator it = list->begin(); it != list->end(); ++it)
	{
		vertex* v = *it;
		if(v->label == label) return true;
	}
	return false;
}

// check for some edge in a list
bool edge_exists(list<edge*>* list, int label)
{
	for(std::list<edge*>::iterator it = list->begin(); it != list->end(); ++it)
	{
		edge* e = *it;
		if(e->label == label) return true;
	}
	return false;
}

// search a list for an edge based on the edge label, returing a pointer for the edge if it is in the list
edge* get_edge(list<edge*>* list, int label)
{
	for(std::list<edge*>::iterator it = list->begin(); it != list->end(); ++it)
	{
		edge* e = *it;
		if(e->label == label) return e;
	}
	return NULL;
}

// this procedure creates the graph used by the main function
// opens, read and process the input file
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

	// process each line of the file
	while(!input_file.eof())
	{
		// put the line in a buffer
		char buffer[256];
		input_file.getline(buffer, sizeof(buffer));

		// check the first letter of the line, witch indicates the type of vertex being described

		// type is input
		if(buffer[0] == 'i')
		{
			// identify the label
			int label_start_index = string(buffer).find("(");
			int label_final_index = string(buffer).find(")");
			int len = label_final_index - label_final_index - 1;
			int new_input_label = stoi(string(buffer).substr(label_start_index+1,len));

			// create the input vertex and add to the vertex list
			vertex* new_input = (vertex*)malloc(sizeof(vertex));
			new_input->label = new_input_label;
			new_input->out_edges = new list<edge*>;
			new_input->in_edges = NULL;
			if(vertex_exists(&vertices, new_input_label))
			{
				cerr << "Failed to add the input vertex to the list. Label " << new_input_label << " already exists." << endl;
				exit(-1);
			}
			else
			{
				vertices.push_back(new_input);
				inputs.push_back(new_input);
			}

			// create and add the edges to the vertex and main edges list
			int outedges_start_index = string(buffer).find("[");
			int outedges_final_index = string(buffer).find("]");
			len = outedges_final_index - outedges_start_index - 1;
			char edges_buf[256];
			strcpy(edges_buf, string(buffer).substr(outedges_start_index+1,len).c_str());
			char* token = strtok(edges_buf,",");
			while(token != NULL)
			{
				edge* new_edge = (edge*)malloc(sizeof(edge));
				new_edge->label = atoi(token);
				new_edge->src = new_input;
				new_edge->dst = NULL;
				new_input->out_edges->push_back(new_edge);
				if(edge_exists(&edges, atoi(token)))
				{
					cerr << "Failed to add the edge to the list. Label " << atoi(token) << " already exists." << endl;
					exit(-1);
				}
				else edges.push_back(new_edge);
				token = strtok(NULL,",");
			}
		}
		
		// type is vertex (a regular vertex, one that is not an input)
		if(buffer[0] == 'v')
		{
			// identify the label
			int label_start_index = string(buffer).find("(");
			int label_final_index = string(buffer).find(")");
			int len = label_final_index - label_final_index - 1;
			int new_vertex_label = stoi(string(buffer).substr(label_start_index+1,len));

			// create the vertex and add to the vertex list
			vertex* new_vertex = (vertex*)malloc(sizeof(vertex));
			new_vertex->label = new_vertex_label;
			new_vertex->out_edges = new list<edge*>;
			new_vertex->in_edges = new list<edge*>;
			if(vertex_exists(&vertices, new_vertex_label))
			{
				cerr << "Failed to add the vertex to the list. Label " << new_vertex_label << " already exists." << endl;
				exit(-1);
			}
			else vertices.push_back(new_vertex);			

			// create and add the edges that leaves the vertex to the vertex and main edges list, update edges that comes to the vertex
			// -- part 1: edges that leaves the vertex
			int outedges_start_index = string(buffer).find("[");
			int outedges_final_index = string(buffer).find("]");
			len = outedges_final_index - outedges_start_index - 1;
			char edges_buf[256];
			strcpy(edges_buf, string(buffer).substr(outedges_start_index+1,len).c_str());
			char* token = strtok(edges_buf,",");
			while(token != NULL)
			{
				edge* new_edge = (edge*)malloc(sizeof(edge));
				new_edge->label = atoi(token);
				new_edge->src = new_vertex;
				new_edge->dst = NULL;
				new_vertex->out_edges->push_back(new_edge);
				if(edge_exists(&edges, atoi(token)))
				{
					cerr << "Failed to add the edge to the list. Label " << atoi(token) << " already exists." << endl;
					exit(-1);
				}
				else edges.push_back(new_edge);
				token = strtok(NULL,",");
			}
			// -- part 2: edges that comes to the vertex 
			int inedges_start_index = string(buffer).find("{");
			int inedges_final_index = string(buffer).find("}");
			len = inedges_final_index - inedges_start_index - 1;
			strcpy(edges_buf, string(buffer).substr(inedges_start_index+1,len).c_str());
			token = strtok(edges_buf,",");
			while(token != NULL)
			{							
				if(!edge_exists(&edges, atoi(token)))
				{
					cerr << "Failed to update an edge. The edge " << atoi(token) << " do not exists." << endl;
					exit(-1);
				}
				else
				{
					edge* e = get_edge(&edges, atoi(token));
					if(e == NULL)
					{
						cerr << "Failed to update an edge. Could not retrieve the edge " << atoi(token) << " from the edges list." << endl;
						exit(-1);
					}
					if(e->dst != NULL)
					{
						cerr << "Failed to update an edge. The edge " << atoi(token) << " already has a destination vertex." << endl;
						exit(-1);
					}
					e->dst = new_vertex;
					new_vertex->in_edges->push_back(e);
				}
				token = strtok(NULL,",");
			}
		}		
	}
}

// this is an auxiliary procedure that prints on screen the graph created, used to debug purposes only
void debug_graph()
{
	for(std::list<vertex*>::iterator it = vertices.begin(); it != vertices.end(); ++it)
	{
		cout << "v(" << (*it)->label << "){";
		if((*it)->in_edges != NULL && (*it)->in_edges->size() > 0)
			for(std::list<edge*>::iterator it2 = (*it)->in_edges->begin(); it2 != (*it)->in_edges->end(); ++it2)
				cout << (*it2)->label << ",";
		cout << "}[";
		if((*it)->out_edges->size() > 0) 
			for(std::list<edge*>::iterator it2 = (*it)->out_edges->begin(); it2 != (*it)->out_edges->end(); ++it2)
				cout << (*it2)->label << ",";
		cout << "]" << endl;
	}
}



/*
 * MAIN FUNCTION: COMPUTES THE PRIORITY K-CUT
 *********************************************/

int main(int argc, char* argv[])
{
	// check for correct usage
	if(argc != 2)
	{
		cerr << "Usage: " << argv[0] << " [input-file]" << endl << "[input-file] = a text file that describes an AIG." << endl;
		return -1;
	}

	// create the AIG graph
	create_graph_from_input_file(argv[1]);
	//debug_graph(); // uncomment this line to show on screen the AIG created based on the input file

	int max_cuts = 2;
	int max_inputs = 3;

	/*
	 * ABOUT THE ALGORITHM
	 *
	 * A "layer" of vertices is a set of vertices witch edges that comes from the preceding layers.
	 * The first layer is the set of inputs. The second layer follows immeadiately the layer of inputs (the both edges that comes in its vertices comes from an input vertex).
	 * The third layer has edges that comes from the vertices of the first or second layer, and so on...
	 * The algorithm evaluates the set of vertices of a layer based on the current_layer.
	 * The algorithm begins making the inputs set the current_layer.
	 * Then, it evaluates the set of vertices from the layer immeadiately above the current_layer (called next_layer).
	 * Then, it evaluates the priority k-cuts and sets current_layer = next_layer.
	 * The algorithm stops when, after evaluating next_layer, the result is an empty set.
	 *
	 */
	
	current_layer = &inputs;
	preceding_vertices = new list<vertex*>;
	for(std::list<vertex*>::iterator it = inputs.begin(); it != inputs.end(); ++it)	preceding_vertices->push_back(*it);

	int ln = 1;

	cout << "Inputs (layer n. " << ln << "):" << endl;
	for(std::list<vertex*>::iterator it = inputs.begin(); it != inputs.end(); ++it) cout << "v(" << (*it)->label << ") " << endl;
	ln++;

	do
	{

		// First, initialize next_layer as an empty set
		next_layer = new list<vertex*>;

		// Then, add in the next_layer all the vertices pointed by the edges that leaves a vertex from the current_layer
		for(std::list<vertex*>::iterator it = current_layer->begin(); it != current_layer->end(); ++it)
		{
			vertex* v = *it;
			// out_edges is the set of edges that leaves the vertex
			for(std::list<edge*>::iterator it = v->out_edges->begin(); it != v->out_edges->end(); ++it)
			{
				edge* e = *it;
				// dst is a pointer for the vertex pointed by the edge
				vertex* edst = (vertex*)e->dst;
				// if the vertex is not yet in the next_layer, adds it to the set
				if(edst != NULL && !vertex_exists(next_layer,edst->label)) next_layer->push_back(edst);
			}
		}

		/*
		 * At this point, next_layer has all the vertices pointed by the edges that leaves the vertices in the current_layer set,
		 * BUT, note that these vertices can be pointed also by any other vertices, including vertices that are not in the
		 * current_layer set or any preceding layer.
		 *
		 * To fix this problem, there is a list with only the vertices that are in the current_layer or in a preceding layer,
		 * called 'preceding_vertices'
		 *
		 * SO, we need to check the edges that comes in to a vertex currently in the next_layer set. If the source of an edge
		 * is not a vertex of preceding_vertices, we need to remove the vertex pointed by this edge from next_layer
		 *
		 */
		
		// first, find the vertices	that must not be in the next_list and put them in a remove_list
		list<vertex*>* remove_list = new list<vertex*>;
		for(std::list<vertex*>::iterator it = next_layer->begin(); it != next_layer->end(); ++it)
		{
			vertex* v = *it;
			for(std::list<edge*>::iterator it = v->in_edges->begin(); it != v->in_edges->end(); ++it)
			{
				edge* e = *it;
				vertex* esrc = (vertex*)e->src;
				if(esrc != NULL && !vertex_exists(preceding_vertices,esrc->label)) remove_list->push_back(v);
			}
		}

		// then, remove the vertices from the next_layer set
		for(std::list<vertex*>::iterator it = remove_list->begin(); it != remove_list->end(); ++it) next_layer->remove(*it);

		cout << "Layer n. " << ln << ":" << endl;
		for(std::list<vertex*>::iterator it = next_layer->begin(); it != next_layer->end(); ++it)	cout << "v(" << (*it)->label << ") " << endl;
		ln++;

		current_layer = next_layer;
		for(std::list<vertex*>::iterator it = next_layer->begin(); it != next_layer->end(); ++it)	preceding_vertices->push_back(*it);

	}
	while(current_layer->size() != 0);

	return 0;
}
