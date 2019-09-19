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
 * DATA STRUCTURES 
 ****************************************/

// structures that forms the AIG graph
typedef struct e {
	int src;
	int dst;
} edge;

typedef struct v {
	int label;
	int i1;
	int i2;
	int winner_cut;
} vertex;

// the cuts
typedef struct c {
	float cost;
	int output;
	int inputs[MAX_INPUTS];
} cut;

// these lists describes the AIG
edge* edges;
vertex* inputs;
vertex* vertices;

// these lists are used by the algorithm to evaluate the results
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
	input_file.getline(buffer, sizeof(buffer));

	// file format check
	if(buffer[0] != 'a' && buffer[1] != 'a' && buffer[2] != 'g')
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
	inputs = (vertex*) malloc ( num_inputs * sizeof(vertex) );
	vertices = (vertex*) malloc ( num_variables * sizeof(vertex) );
	edges = (edge*) malloc ( num_ands * sizeof(edge) );

	// Creates the input vertices and their outcoming edges
	for(int i = 0; i < num_inputs; i++)
	{
		vertex* v = inputs + i*sizeof(vertex);
		input_file.getline(buffer, sizeof(buffer));
		token = strtok(buffer, " ");
		v->label = atoi(token);
		v->i1 = -1;
		v->i2 = -1;
		v->winner_cut = -1;
	}
	for(int i = 0; i < num_inputs; i++)
	{
		vertex* v = inputs + i*sizeof(vertex);
		cout << i << ": label = " << v->label << ", i1 = " << v->i1 << ", i2 = " << v->i2 << ", winner_cut = " << v->winner_cut << endl;
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

	return 0;
}
