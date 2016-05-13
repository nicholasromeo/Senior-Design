#ifndef GRAPH_DATA_H
#define GRAPH_DATA_H

#include <stdint.h>

typedef struct
{
	int id;
	int automorphism_index;
	int num_automorphisms;
	int8_t facets[24][3];
	uint64_t hash;
	int8_t canonical_labelling[15];
} graph_t;

#define NUM_SC_GRAPHS 1
#define NUM_ICO_GRAPHS 1
#define NUM_FCC_GRAPHS 8
#define NUM_HCP_GRAPHS 16
#define NUM_BCC_GRAPHS 218

extern int8_t automorphisms[][15];

extern graph_t graphs_sc[NUM_SC_GRAPHS];
extern graph_t graphs_fcc[NUM_FCC_GRAPHS];
extern graph_t graphs_hcp[NUM_HCP_GRAPHS];
extern graph_t graphs_ico[NUM_ICO_GRAPHS];
extern graph_t graphs_bcc[NUM_BCC_GRAPHS];

#endif

