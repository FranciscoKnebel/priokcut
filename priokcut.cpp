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

list<edge*> edges;
list<vertex*> inputs;
list<vertex*> vertices;

bool vertexExists(list<vertex*>* list, int label)
{
	for(std::list<vertex*>::iterator it = list->begin(); it != list->end(); ++it)
	{
		vertex* v = *it;
		if(v->label == label) return true;
	}
	return false;
}

bool edgeExists(list<edge*>* list, int label)
{
	for(std::list<edge*>::iterator it = list->begin(); it != list->end(); ++it)
	{
		edge* e = *it;
		if(e->label == label) return true;
	}
	return false;
}

int main(int argc, char* argv[])
{
	// check for correct usage
	if(argc != 2)
	{
		cerr << "Usage: " << argv[0] << " [input-file]" << endl << "[input-file] = a text file that describes an AIG." << endl;
		return -1;
	}

	// opens the input file	
	ifstream input_file;
	input_file.open(argv[1], ifstream::in);

	if(!input_file.is_open())
	{
		cerr << "Failed to open the input file." << endl;
		return -1;
	}

	// process each line
	while(!input_file.eof())
	{
		char buffer[256];
		input_file.getline(buffer, sizeof(buffer));

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
			if(vertexExists(&vertices, new_input_label))
			{
				cerr << "Failed to add the input vertex to the list. Label " << new_input_label << " already exists." << endl;
				return -1;
			}
			else vertices.push_back(new_input);			

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
				if(edgeExists(&edges, atoi(token)))
				{
					cerr << "Failed to add the edge to the list. Label " << atoi(token) << " already exists." << endl;
					return -1;
				}
				else edges.push_back(new_edge);
				token = strtok(NULL,",");
			}
		}
	}
	return 0;
}
