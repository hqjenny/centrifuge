//#include<stdlib.h>
#include<stdint.h>
#include<stdio.h>
//#include<stdbool.h>

#define label_t uint64_t
#define bit_t uint32_t

#define MAX_EDGES 131072
#define MAX_VERTICES 8192


//typedef struct device_status
//  {
//   // unsigned char done : 1;
//   // unsigned char converged : 1;
//   // unsigned char output_valid : 1;
//   // unsigned char input_valid : 1;
//   // unsigned char bit5 : 1;
//   // unsigned char bit6 : 1;
//   // unsigned char bit7 : 1;
//   // unsigned char bit8 : 1;
//	bit_t done; // input from CPU
//	bit_t converged;
//	bit_t output_valid;
//	bit_t input_valid;
//
//  } device_status_t;

typedef struct ctrl {
  uint32_t done;
  uint32_t converged;
  uint32_t input_valid;
  uint32_t output_valid;
	size_t output_size;
	size_t input_size;
  //ap_uint<32> status;
  //uint32_t input_size;
  //uint32_t output_size;
} ctrl_t;

typedef struct edge {
    uint64_t from;
    uint64_t to;
} edge_t;

typedef struct info {
	uint64_t to;
	uint64_t label;
} info_t;

//void top (ctrl_t* ctrl, edge_t* edges, info_t* input, info_t* output, label_t* labels,
//		int world_rank, int world_size, int num_edges, int num_vertices);

int vtor(int vertex_idx, int world_size, int num_vertices) {
  int trunk_size =  (num_vertices + world_size - 1) / world_size;
  return vertex_idx / trunk_size;
  //return ((vertex_idx + 1) * world_size / num_vertices;
}

int rtov_upper(int rank, int world_size, int num_vertices) {
  int trunk_size =  (num_vertices + world_size - 1) / world_size;
  int upper = ((rank+1) * trunk_size < num_vertices) ? (rank+1) * (trunk_size + 1): num_vertices;
  //return num_vertices * (rank + 1) / world_size;
  printf("%d %d %d %d %d\n", rank, world_size, num_vertices, trunk_size, upper);
  return upper;
}

int rtov_lower(int rank, int world_size, int num_vertices) {
  int trunk_size =  (num_vertices + world_size - 1) / world_size;
  int lower = rank * (trunk_size) < num_vertices ? rank * (trunk_size): num_vertices;
  return lower;
  //return num_vertices * rank / world_size;
}

