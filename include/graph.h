#ifndef GRAPH_H
#define GRAPH_H

/* DATA STRUCTURES
******************************************************************************/

typedef struct v {
	int left;
	int right;
	int fanout;
    int winner;
} vertex;

#endif