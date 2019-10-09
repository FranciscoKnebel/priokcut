///////////////////////////////////////////////////////////////////////////////
//
// Company: Universidade Federal do Rio Grande do Sul
// Engineer: Rafael de Oliveira Calçada
//
// Create Date: 19.09.2019 16:12:57
// Description: Priority K-cuts Algorithm Implementation
//
// Revision:
// Revision 3.0 - Performance enhancement #2
//
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <vector>
#include <stack>
#include <ctime>
#include "graph.h"
using namespace std;

/* GLOBALS
******************************************************************************/

// AIG data
vertex* vertices = NULL;
int* outputs = NULL;
int M = 0;
int I = 0;
int L = 0;
int O = 0;
int A = 0;

// algorithm parameters
int p = 0;
int k = 0;
bool display = false;
char* filename = NULL;

// data used to compute the results
stack<int, vector<int>>* stk;
float* cut_costs;
int* cut_inputs;

// functions and procedures implemented elsewhere
void process_args(int argc, char* argv[], char*& filename, bool& display, int& p, int& k);
void show_help(char* argv[]);
void create_graph(char* filename, int& M, int& I, int& L, int& O, int& A, vertex*& vertices, int*& outputs);
void evaluate_time(const char* message, clock_t& start, clock_t& end);
void print_cuts(int vertex_index, int& p, int& k, float*& cut_costs, int*& cut_inputs);
bool in_the_list(int vertex_index, vector<int>* list);
int winner_cut(int vertex_index, float* cut_costs, int& p);
int loser_cut(int vertex_index, float* cut_costs, int& p);

/* COMPUTES THE K-CUTS FOR A VERTEX
 * Return value: the index of the winner cut
*****************************************************************************/
int compute_kcuts(int vertex_index, int left_index, int right_index, int input_offset)
{

    int winner_index = 0;

    // initialize the cuts of the vertex
    for (int j = 0; j < p; j++)
    {
        cut_costs[vertex_index * p + j] = -1.0;
        for (int l = 0; l < k; l++)
            cut_inputs[vertex_index * input_offset + j * k + l] = -1;
    }

    // computes the cartesian product
    vector<int> product;
    float product_cost;
    float left_cut_cost;
    float right_cut_cost;

    for (int j = 0; j < p; j++)
    {
        for (int z = 0; z < p; z++)
        {

            product.clear();

            // EVALUATES THE COST OF THE PRODUCT
            // the cost of a given product is equal the sum osum of the costsum of the costs
            // of each cut divided by the fanout
            if (left_index >= 0)
                left_cut_cost = cut_costs[left_index * p + j];
            else
                left_cut_cost = 0.0;
            if (right_index >= 0)
                right_cut_cost = cut_costs[right_index * p + z];
            else
                right_cut_cost = 0.0;
            if (vertices[vertex_index].fanout == 0)
            {
                cerr << "Found a vertex (" << (vertex_index + 1) * 2 << ") with fanout = 0." << endl;
                exit(-1);
            }
            else
                product_cost = (left_cut_cost + right_cut_cost) / (float)vertices[vertex_index].fanout;

            // COMPUTES THE PRODUCT
            // a negative number for the cost is a flag for an empty space left
            // in the cost vector. So if one of the cuts has a negative value,
            // jump for the next
            if (left_cut_cost < 0 || right_cut_cost < 0)
                continue;
            else
            {
                if (left_index >= 0)
                {
                    for (int l = 0; l < k; l++)
                        if (cut_inputs[left_index * input_offset + j * k + l] != -1)
                            product.push_back(cut_inputs[left_index * input_offset + j * k + l]);
                }
                else product.push_back(-(left_index + 2));
                if (right_index >= 0)
                {
                    for (int l = 0; l < k; l++)
                        if (!in_the_list(cut_inputs[right_index * input_offset + z * k + l], &product) && cut_inputs[right_index * input_offset + z * k + l] != -1)
                            product.push_back(cut_inputs[right_index * input_offset + z * k + l]);
                }
                else product.push_back(-(right_index + 2));

                // If the product has a lower cost than some cut placed
                // in the vertex cuts list before, replaces the cut
                // If there is an empty space in the vertex's cuts list,
                // place the product in the list
                // If the product has more than k inputs, it is discarded
                if (product.size() <= k)
                {
                    for (int l = 0; l < p; l++)
                    {
                        int actual_cost = cut_costs[vertex_index * p + l];
                        if (actual_cost == -1 || actual_cost > product_cost)
                        {
                            cut_costs[vertex_index * p + l] = product_cost;
                            for (int m = 0; m < k; m++)
                                cut_inputs[vertex_index * input_offset + l * k + m] = -1;
                            for (int m = 0; m < product.size(); m++)
                                cut_inputs[vertex_index * input_offset + l * k + m] = product.at(m);
                            break;
                        }
                    }
                }
            }
        }
    }

    // evaluates the cost of the autocut
    winner_index = winner_cut(vertex_index, cut_costs, p);
    float winner_cost = cut_costs[winner_index];
    float autocut_cost = winner_cost + (1.0 / (float)vertices[vertex_index].fanout);

    // inserts the autocut in a free position (if there is), OR
    // replaces a cut "worse" than the autocut (a cut with higher cost)
    bool replaced_or_inserted = false;
    for (int l = 0; l < p; l++)
        if (!replaced_or_inserted)
        {
            int actual_cost = cut_costs[vertex_index * p + l];
            if (actual_cost == -1 || actual_cost > autocut_cost)
            {
                cut_costs[vertex_index * p + l] = autocut_cost;
                for (int m = 0; m < k; m++)
                    cut_inputs[vertex_index * input_offset + l * k + m] = -1;
                cut_inputs[vertex_index * input_offset + l * k] = (vertex_index + 1) * 2;
                replaced_or_inserted = true;
            }
        }

    // if there's no free position and no cut is worse than the autocut,
    // chooses the cut with the highest cost ("loser" cut) and replaces it
    if (!replaced_or_inserted)
    {
        int loser_cut_index = loser_cut(vertex_index, cut_costs, p);
        cut_costs[vertex_index * p + loser_cut_index] = autocut_cost;
        for (int m = 0; m < k; m++)
            cut_inputs[vertex_index * input_offset + loser_cut_index * k + m] = -1;
        cut_inputs[vertex_index * input_offset + loser_cut_index * k] = (vertex_index + 1) * 2;
    }

    return winner_index;
}

/* MAIN FUNCTION: COMPUTES THE PRIORITY K-CUTS FOR A GIVEN AIG
******************************************************************************/
int main(int argc, char* argv[])
{

    // set default values
	p = 2;
	k = 4;
	display = false;

    // check for correct usage
	if(argc < 2)
	{
		show_help(argv);
		return -1;
	}

    // process the arguments
    process_args(argc, argv, filename, display, p, k);

    // initializes the time counter
	clock_t execution_start = clock();

    // creates the graph
    create_graph(filename, M, I, L, O, A, vertices, outputs);

    // evaluates the time taken to create and load the graph in the main memory
    clock_t end = clock();
    evaluate_time("Time taken to load the AIG in the main memory: ", execution_start, end);

    /* ABOUT THE ALGORITHM
     *
     * At start, the cost of all input vertices are set to zero.
     * Then, for each output vertex, the algorithm iteratively checks if the
     * left and right child vertices have non null cost. If yes,
     * evaluate the priority k-cuts based on the child cuts.
     * If not, stack up the vertex and repeat the test for the child
     * vertex that has the null cost.
     * The iteration stops when the stack is empty.
     *************************************************************************/

	clock_t computation_start = clock();

    // allocates memory for the cuts
    int cost_offset = p;
	int input_offset = p*k;
	cut_costs = new float[M*p];
	cut_inputs = new int[M*p*k];

    // set to zero the cost of each input vertex cut
	// fill blank spaces left in the vector with -1
    // set the winner cut of each input
	for(int i = 0; i < I; i++)
	{
        vertices[i].winner = i;
		cut_costs[(i*cost_offset)] = 0;
		cut_inputs[i*input_offset] = (i+1) << 1;
		for(int l = 1; l < k; l++) cut_inputs[i*input_offset+l] = -1;
		for(int j = 1; j < p; j++)
		{
			cut_costs[(i*cost_offset) + j] = -1;
			for(int l = 0; l < k; l++) cut_inputs[i*input_offset+j*k+l] = -1;
		}
	}

    // allocates the auxiliary stack
    stk = new stack<int, std::vector<int>>;

	/* EVALUATE THE PRIORITY K-CUTS
	**************************************************************************/
    for(int i = 0; i < O; i++)
    {
        int vertex_index = (outputs[i] >> 1) - 1;
        vertex* v = &vertices[vertex_index];
        int left_index = v->left;
        int right_index = v->right;
        vertex* left = &vertices[left_index];
        vertex* right = &vertices[right_index];
        while(v != NULL)
        {
            int left_index = v->left;
            int right_index = v->right;
            vertex* left = &vertices[left_index];
            vertex* right = &vertices[right_index];
            if(left != NULL && left->winner == -1)
            {
                stk->push(vertex_index);
                v = left;
                vertex_index = left_index;
            }
            else if(right != NULL && right->winner == -1)
            {
                stk->push(vertex_index);
                v = right;
                vertex_index = right_index;
            }
            else
            {   
                v->winner = compute_kcuts(vertex_index, left_index, right_index, input_offset);
                if(stk->empty()) v = NULL;
                else
                {
                    vertex_index = stk->top();
                    stk->pop();
                    v = &vertices[vertex_index];
                }
            }
        }
    }

    if(display) for(int i = 0; i < M; i++) print_cuts(i, p, k, cut_costs, cut_inputs);

    // evaluates the time taken to evaluate the priority k-cuts
    end = clock();
    evaluate_time("Time taken to evaluate the priority k-cuts: ", computation_start, end);
    evaluate_time("Total execution time: ", execution_start, end);

    return 0;

}