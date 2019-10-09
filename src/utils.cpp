#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <vector>
#include "graph.h"
using namespace std;

// evaluates and show the time taken to do something
void evaluate_time(const char* message, clock_t& start, clock_t& end)
{
    double time_spent = 0.0;
	time_spent += ((double)(end - start) / CLOCKS_PER_SEC);
	cout.setf(std::ios::fixed);
	string time_sec = to_string(time_spent);
	string time_msec = to_string((time_spent*1000.0));
	string time_usec = to_string((time_spent*1000000.0));
	time_sec.erase(time_sec.find_last_not_of('0'), string::npos);
	time_msec.erase(time_msec.find_last_not_of('0'), string::npos);
	time_usec.erase(time_usec.find_last_not_of('0'), string::npos);
    cout << endl << message << endl;
	cout << endl << "In seconds:       " << time_sec << " s" << endl;
	cout << "In milisseconds:  " << time_msec << " ms" << endl;
	cout << "In microsseconds: " << time_usec << " us" << endl << endl;   
}

// show the help on screen
void show_help(char* argv[])
{
		cerr << endl << "  \e[1mUsage:\e[0m " << argv[0] << " <file> [options]" << endl << endl;
		cerr << "  <file>         An AIG in the binary or ASCII format." << endl;
        cerr << "                 This argument is required." << endl << endl;
		cerr << "  \e[1mOptions\e[0m:" << endl << endl;
		cerr << "  -k <value>     The maximum number of inputs for each cut." << endl;
		cerr << "  -p <value>     The number of prioriry cuts stored for each vertex." << endl;
		cerr << "  -d             Display the results on the screen (slow down the execution time" << endl;
        cerr << "                 for large graphs)." << endl << endl;
		cerr << "  -h --help      This help." << endl << endl;
		cerr << "  If not provided, the values of p and k are set to 2 and 4, respectively, and " << endl;
        cerr << "  display is set to false." << endl << endl;
}

// process the arguments passed by command line interface
void process_args(int argc, char* argv[], char*& filename, bool& display, int& p, int& k)
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
			else if(arg[1] == 'd')
			{
				display = true;
				i++;
			}
			else if(arg[1] == 'h')
			{
				show_help(argv);
				exit(-1);
			}
			else if(arg[1] == '-')
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
			else if(arg[1] == 'k' || arg[1] == 'p')
			{
				if(i+1 < argc)
				{
					char* nextarg = argv[i+1];
					if(nextarg[0] == '-')
					{
						cerr << "FAIL. Missing or wrong value for -" << arg[1] << " option." << endl;
						exit(-1);						
					}
					if(arg[1] == 'k') k = atoi(nextarg);
					if(arg[1] == 'p') p = atoi(nextarg);
					i += 2;
				}
				else
				{
					cerr << "FAIL. Missing or wrong value for -" << arg[1] << " option." << endl;
					exit(-1);
				}
			}
            else
            {
				cerr << "FAIL. Unknown option." << endl;
				exit(-1);					
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

	if(p < 2 || k < 2)
	{
		cerr << "FAIL. Minimal value for -k and -p is 2." << endl;
		exit(-1);
	}

}

// prints the cuts of a vertex on screen
void print_cuts(int vertex_index, int& p, int& k, float*& cut_costs, int*& cut_inputs)
{
	int vertex_label = (vertex_index+1) << 1;
	cout << "v[" << vertex_label << "] cuts:" << endl;
	for(int i = 0; i < p; i++)
	{
		if(cut_costs[vertex_index*p+i] != -1) {
			cout << "  {'cut':{";
			for(int j = 0; j < k; j++)
            {
				if(cut_inputs[vertex_index*p*k+i*k+j] != -1)
                {
                    cout << cut_inputs[vertex_index*p*k+i*k+j];
                    if(j < k-1 && cut_inputs[vertex_index*p*k+i*k+j+1] != -1) cout << ",";
                }
            }
			cout << "},'cost':" << cut_costs[vertex_index*p+i] << "}" << endl;
		}
	}
}

// check if a vertex is in the list
bool in_the_list(int vertex_index, vector<int>* list)
{
	for(int i = 0; i < list->size(); i++)
		if(list->at(i) == vertex_index) 
			return true;
	return false;
}

// return the index of the winning cut in the cut's cost list
int winner_cut(int vertex_index, float* cut_costs, int& p)
{
	int winner_cost = cut_costs[vertex_index*p];
	int winner_index = vertex_index*p + 0;
	for(int j = 0; j < p; j++)
	{
		if(cut_costs[vertex_index*p + j] < 0) continue;
		if(winner_cost >= cut_costs[vertex_index*p + j] ||
		cut_costs[vertex_index*p + j] > 0 && winner_cost < 0)
		{
			winner_cost = cut_costs[vertex_index*p + j];
			winner_index = vertex_index*p + j;
		}
	}
	return winner_index;
}

// return the index of the loser cut in the vertex cut list
int loser_cut(int vertex_index, float* cut_costs, int& p)
{
	float loser_cost = 0;
	int loser_index = 0;
	for(int j = 0; j < p; j++)
	{
		if(loser_cost <= cut_costs[vertex_index*p + j])
		{
			loser_cost = cut_costs[vertex_index*p + j];
			loser_index = j;
		}
	}
	return loser_index;
}