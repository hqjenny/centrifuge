#include "graph.h"
#include "hls_stream.h"
#include "ap_int.h"

//1. All edges point to me
//2. 4 parts of vertices
//	a. myself-myself
//	b. send the data from the proc2/proc3/proc4 to me
//	c. process the lables


// Convert directed graph -> undirected graph
// strongly connected components -> weakly connected components (Parconnect)
// adding opposite direction to the graph data
// ctrl[0] -> accel output valid signal
// ctrl[1] -> accel input valid signal
// output_size -> largest buffer size, but we are not using it
//void top(ctrl_t* ctrl, edge_t* edges, info_t* input, info_t* output, label_t*
//void top(int* ctrl_done, int* ctrl_converged, int* ctrl_input_valid, int* ctrl_output_valid, 
void top(volatile int* ctrl_done, volatile int* ctrl_converged, volatile int* ctrl_input_valid, volatile int* ctrl_output_valid, 
    volatile int* ctrl_output_size, volatile int* ctrl_input_size,
    edge_t* edges, info_t* input, info_t* output, label_t*
		labels, int world_rank, int world_size, int num_edges,
		int num_vertices) {
#pragma HLS data_pack variable=edges struct_level
#pragma HLS data_pack variable=input struct_level
#pragma HLS data_pack variable=output struct_level
#pragma HLS data_pack variable=labels struct_level

#pragma HLS INTERFACE s_axilite port=return bundle=control
#pragma HLS INTERFACE s_axilite port=edges bundle=control

#pragma HLS INTERFACE m_axi port=ctrl_done offset=slave depth=128 bundle=gmem1
#pragma HLS INTERFACE s_axilite port=ctrl_done bundle=control
#pragma HLS INTERFACE m_axi port=ctrl_converged offset=slave depth=128 bundle=gmem1
#pragma HLS INTERFACE s_axilite port=ctrl_converged bundle=control
#pragma HLS INTERFACE m_axi port=ctrl_input_valid offset=slave depth=128 bundle=gmem1
#pragma HLS INTERFACE s_axilite port=ctrl_input_valid bundle=control
#pragma HLS INTERFACE m_axi port=ctrl_output_valid offset=slave depth=128 bundle=gmem1
#pragma HLS INTERFACE s_axilite port=ctrl_output_valid bundle=control
#pragma HLS INTERFACE m_axi port=ctrl_output_size offset=slave depth=128 bundle=gmem1
#pragma HLS INTERFACE s_axilite port=ctrl_output_size bundle=control
#pragma HLS INTERFACE m_axi port=ctrl_input_size offset=slave depth=128 bundle=gmem1
#pragma HLS INTERFACE s_axilite port=ctrl_input_size bundle=control

#pragma HLS INTERFACE m_axi port=edges offset=slave depth=128 bundle=gmem0
#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE m_axi port=input offset=slave depth=128 bundle=gmem0
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE m_axi port=output offset=slave depth=128 bundle=gmem0
#pragma HLS INTERFACE s_axilite port=labels bundle=control
#pragma HLS INTERFACE m_axi port=labels offset=slave depth=128 bundle=gmem2

#pragma HLS INTERFACE s_axilite port=world_rank bundle=control
#pragma HLS INTERFACE s_axilite port=world_size bundle=control
#pragma HLS INTERFACE s_axilite port=num_edges bundle=control
#pragma HLS INTERFACE s_axilite port=num_vertices bundle=control

	bit_t converged = 1;
	label_t local_labels[MAX_VERTICES];
	//#pragma HLS ARRAY_PARTITION variable=local_labels complete dim=0

	int offset = rtov_lower(world_rank, world_size, num_vertices);

  int upper = rtov_upper(world_rank, world_size, num_vertices);
	for (int i = offset;
			i < upper; i++) {
		//#pragma HLS UNROLL factor=16
		local_labels[i - offset] = i;
	}

	while (1) {

		int count = 0;
		converged = 1;
		for (int i = 0; i < num_edges; i++) {
			//#pragma HLS UNROLL factor=16
			edge_t e = edges[i];

			int rto = vtor(e.to, world_size, num_vertices);
			int rfrom = vtor(e.from, world_size, num_vertices);

			// TODO: partition the edges so that the "from" rank is
			// the same for a single node

			if (rto != world_rank && rfrom == world_rank) {
				info_t info;
				info.to = e.to;
				info.label = local_labels[e.from - offset];
				// destination vertex + label of rfrom if we have more than 2 processors, we need to have more buffers
				output[count++] = info;
			} else if (rto == world_rank && rfrom == world_rank) {
				if (local_labels[e.from - offset]
						< local_labels[e.to - offset]) {
					local_labels[e.to - offset] = local_labels[e.from - offset];
					converged = 0;
				}
			}
		}

		*ctrl_output_size = count;

		*ctrl_converged = converged;

		*ctrl_output_valid = 1;

		count = 0;
    volatile char sleep = 0;
		while (!(*ctrl_input_valid)) {
        sleep = 0;
        for(int i; i < 10; i++) {
          sleep +=1;
        }
		}

		// process incoming data
		size_t input_size = *ctrl_input_size;
		for (size_t i = 0; i < input_size; i++) {
			info_t info = input[i];
			if (local_labels[info.to - offset] > info.label) {
				local_labels[info.to - offset] = info.label;
				//    		label_updates[in.to - offset] = 1;
				converged = 0;
			}
		}

		/* printf("top processed incoming data\n"); */
		*ctrl_input_valid = 0;

		// wait till send is done on the proc end
    sleep = 0;
		while ((*ctrl_output_valid)) {
        sleep = 0;
        for(int i; i < 10; i++) {
          sleep +=1;
        }
		}


		if (*ctrl_done) {
			for (int i = offset;
					i < upper; i++) {
#pragma HLS UNROLL factor=16
				labels[i] = local_labels[i - offset];
			}
			return;
		}
	}
}
