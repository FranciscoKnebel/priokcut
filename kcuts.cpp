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
#include <fstream>
#include <string.h>
#include <vector>
using namespace std;

#define MAX_CUTS 2
#define MAX_INPUTS 3

/*
 * GENERAL INFO
 ********************************************************************************/

/*	MEMORY USAGE

		8 * A bytes for the edges
		8 * M bytes for the vertices
		(4 + 4 * MAX_INPUTS) * MAX_CUTS * M bytes for the cuts	*/

/*	ALGORITHM CAPACITY

		An AIG graph with up to 1.073.741.824 vertices. For such AIG,
		the algorithm uses 20 MBytes + 4*MAX_CUTS*MAX_INPUTS Mbytes of RAM */


/*
 * DATA STRUCTURES
 ********************************************************************************/

// structures that forms the AIG graph
typedef struct e {
	int src;
	int dst;
} edge;

typedef struct v {
	int i1;
	int i2;
} vertex;

// the cuts
typedef struct c {
	float cost;
	int inputs[MAX_INPUTS];
} cut;

// these 'lists' describes the AIG
vertex* vertices;
edge* edges;
int* outputs;

// these 'lists' are used by the algorithm to evaluate the results
cut* cuts;
vector<vertex>* next_layer;
vector<vertex>* current_layer;
vector<vertex>* preceding_vertices;


/*
 * HELPFUL FUNCTIONS AND PROCEDURES
 ***********************************/


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
		cerr << "Failed to process the input file. Wrong or unknown format." << endl;
		exit(-1);
	}

	// split into tokens, saving the values in variables
	char* token = strtok(buffer," ");
	token = strtok(NULL, " ");
	int num_variables = atoi(token);
	token = strtok(NULL, " ");
	int num_inputs = atoi(token);
	token = strtok(NULL, " ");
	int num_latches = atoi(token);
	token = strtok(NULL, " ");
	int num_outputs = atoi(token);
	token = strtok(NULL, " ");
	int num_ands = atoi(token);

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
	outputs = (int*) malloc ( num_outputs * sizeof(int) );
	vertices = (vertex*) malloc ( num_variables * sizeof(vertex) );
	edges = (edge*) malloc ( num_ands * sizeof(edge) );
	cuts = (cut*) malloc ( num_variables * sizeof(cut) * MAX_CUTS );

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
		// if reached here, everything is OK, so sets the value of the incoming edges as -1 (indicating that has no incoming edges)
		vertex* v = vertices + i*sizeof(vertex);
		v->i1 = -1;
		v->i2 = -1;
	}

	// creates the output vertices
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

	// creates the vertices
	for(int i = num_inputs; i < num_variables; i++)
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
		if(label != ((i+1)*2))
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
		vertex* v = vertices + i*sizeof(vertex);
		v->i1 = i1;
		v->i2 = i2;
	}
}


/*
 * MAIN FUNCTION: COMPUTES THE PRIORITY K-CUTS FOR THE AIG
 ********************************************************************************/

int main(int argc, char* argv[])
{
	// check for correct usage
	if(argc != 2)
	{
		cerr << "Usage: " << argv[0] << " [input-file]" << endl << "[input-file] = an ASCII file that describes an AIG." << endl;
		return -1;
	}

	// create the AIG graph
	create_graph_from_input_file(argv[1]);

	return 0;

}
