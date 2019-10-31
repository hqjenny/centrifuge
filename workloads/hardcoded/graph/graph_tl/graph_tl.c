//#include<stdint.h>
//#include<stdio.h>
#include"graph.h"
#include "time.h"

#include "/scratch/qijing.huang/firesim_new/hls/sw/bm//mmio.h"
#define ACCEL_BASE 0x20000
#define AP_DONE_MASK 0b10
#define ACCEL_INT 0x4
#define ACCEL_ctrl_done_0 0x10
#define ACCEL_ctrl_done_1 0x14
#define ACCEL_ctrl_converged_0 0x1c
#define ACCEL_ctrl_converged_1 0x20
#define ACCEL_ctrl_input_valid_0 0x28
#define ACCEL_ctrl_input_valid_1 0x2c
#define ACCEL_ctrl_output_valid_0 0x34
#define ACCEL_ctrl_output_valid_1 0x38
#define ACCEL_ctrl_output_size_0 0x40
#define ACCEL_ctrl_output_size_1 0x44
#define ACCEL_ctrl_input_size_0 0x4c
#define ACCEL_ctrl_input_size_1 0x50
#define ACCEL_edges_0 0x58
#define ACCEL_edges_1 0x5c
#define ACCEL_input_r_0 0x64
#define ACCEL_input_r_1 0x68
#define ACCEL_output_r_0 0x70
#define ACCEL_output_r_1 0x74
#define ACCEL_labels_V_0 0x7c
#define ACCEL_labels_V_1 0x80
#define ACCEL_world_rank_0 0x88
#define ACCEL_world_size_0 0x90
#define ACCEL_num_edges_0 0x98
#define ACCEL_num_vertices_0 0xa0

void graph (int world_rank, int world_size, int num_vertices, int num_edges, label_t* labels, edge_t* edges) {
  int converged =1;
	int offset = rtov_lower(world_rank, world_size, num_vertices);
	int upper = rtov_upper(world_rank, world_size, num_vertices);
	int num_labels = upper - offset;

	label_t local_labels[MAX_VERTICES/2];

	for (int i = 0; i < num_labels; i++) {
		labels[i] = i;
	}

	int iter = 0;
	while (1) {
		converged = 1;
		int count = 0;
		for (int i = 0; i < num_edges; i++) {
			edge_t e = edges[i];
			printf("cpu: update from %lu(%lu) to %lu(%lu)\n", e.to, local_labels[e.to - offset], e.from, local_labels[e.from - offset]);
			if (labels[e.from] < labels[e.to]) {
				labels[e.to] = local_labels[e.from];
				converged = 0;
			}
		}

		printf("cpu: converged = %i\n", converged);

		if (converged) {
			printf("cpu: done <- true\n");
			break;
		}

		iter++;
	}

	printf("cpu: done\n");
}


void top_wrapper(uint64_t ctrl_done, uint64_t ctrl_converged, uint64_t ctrl_input_valid, uint64_t ctrl_output_valid, uint64_t ctrl_output_size, uint64_t ctrl_input_size, uint64_t edges, uint64_t input_r, uint64_t output_r, uint64_t labels_V, uint32_t world_rank, uint32_t world_size, uint32_t num_edges, uint32_t num_vertices,
 ctrl_t* ctrl) {

    //printf("Start Accel\n");
    // Disable Interrupt
    reg_write32(ACCEL_BASE + ACCEL_INT, 0x0);
    reg_write32(ACCEL_BASE + ACCEL_ctrl_done_0, (uint32_t) (ctrl_done));
    reg_write32(ACCEL_BASE + ACCEL_ctrl_done_1, (uint32_t) (ctrl_done >> 32));
    reg_write32(ACCEL_BASE + ACCEL_ctrl_converged_0, (uint32_t) (ctrl_converged));
    reg_write32(ACCEL_BASE + ACCEL_ctrl_converged_1, (uint32_t) (ctrl_converged >> 32));
    reg_write32(ACCEL_BASE + ACCEL_ctrl_input_valid_0, (uint32_t) (ctrl_input_valid));
    reg_write32(ACCEL_BASE + ACCEL_ctrl_input_valid_1, (uint32_t) (ctrl_input_valid >> 32));
    reg_write32(ACCEL_BASE + ACCEL_ctrl_output_valid_0, (uint32_t) (ctrl_output_valid));
    reg_write32(ACCEL_BASE + ACCEL_ctrl_output_valid_1, (uint32_t) (ctrl_output_valid >> 32));
    reg_write32(ACCEL_BASE + ACCEL_ctrl_output_size_0, (uint32_t) (ctrl_output_size));
    reg_write32(ACCEL_BASE + ACCEL_ctrl_output_size_1, (uint32_t) (ctrl_output_size >> 32));
    reg_write32(ACCEL_BASE + ACCEL_ctrl_input_size_0, (uint32_t) (ctrl_input_size));
    reg_write32(ACCEL_BASE + ACCEL_ctrl_input_size_1, (uint32_t) (ctrl_input_size >> 32));
    reg_write32(ACCEL_BASE + ACCEL_edges_0, (uint32_t) (edges));
    reg_write32(ACCEL_BASE + ACCEL_edges_1, (uint32_t) (edges >> 32));
    reg_write32(ACCEL_BASE + ACCEL_input_r_0, (uint32_t) (input_r));
    reg_write32(ACCEL_BASE + ACCEL_input_r_1, (uint32_t) (input_r >> 32));
    reg_write32(ACCEL_BASE + ACCEL_output_r_0, (uint32_t) (output_r));
    reg_write32(ACCEL_BASE + ACCEL_output_r_1, (uint32_t) (output_r >> 32));
    reg_write32(ACCEL_BASE + ACCEL_labels_V_0, (uint32_t) (labels_V));
    reg_write32(ACCEL_BASE + ACCEL_labels_V_1, (uint32_t) (labels_V >> 32));
    reg_write32(ACCEL_BASE + ACCEL_world_rank_0, (uint32_t) (world_rank));
    reg_write32(ACCEL_BASE + ACCEL_world_size_0, (uint32_t) (world_size));
    reg_write32(ACCEL_BASE + ACCEL_num_edges_0, (uint32_t) (num_edges));
    reg_write32(ACCEL_BASE + ACCEL_num_vertices_0, (uint32_t) (num_vertices));

	int converged = 1;
	int world_size_cpu = 2;
	int world_rank_cpu = 1;
//	int num_edges = num_edges;
//	int num_vertices = num_vertices;

	int offset = rtov_lower(world_rank_cpu, world_size_cpu, num_vertices);
	int upper = rtov_upper(world_rank_cpu, world_size_cpu, num_vertices);
	int num_labels = upper - offset + 1;

  info_t* input= (info_t*)input_r;
  info_t* output = (info_t*)output_r; 
	edge_t* edges_ptr = (edge_t*)edges;

  label_t local_labels[MAX_VERTICES/2+1];
  label_t* labels =  (label_t*)labels_V;
  printf("num_labels %d\n", num_labels);
  printf("num_edges %d\n", num_edges);
  printf("num_vertices%d\n",num_vertices);
  printf("offset%d\n",offset);
  printf("upper%d\n",upper);
  for (int i = 0; i < num_labels; i++) {
    //local_labels[i] = labels[i + offset];
    local_labels[i] = i + offset;

  }


  *((uint32_t*)ctrl_done) = 0;
	*((uint32_t*)ctrl_input_valid) = 0;
//	ctrl_t *ctrl = ctrl;
    // Write to ap_start to start the execution 
    reg_write32(ACCEL_BASE, 0x1);

	while (1) {

		converged = 1;
		int count = 0;
		for (int i = 0; i < num_edges; i++) {
			edge_t e = edges_ptr[i];

			int rto = vtor(e.to, world_size, num_vertices);
			int rfrom = vtor(e.from, world_size, num_vertices);
      printf("%d rfrom%d(%d) rto%d(%d)\n",i, rfrom, e.from, rto, e.to);
			if (rto != world_rank_cpu && rfrom == world_rank_cpu) {
				info_t info;
				info.to = e.to;
				info.label = local_labels[e.from - offset];
				input[count++] = info;
        printf("cpu: send from %d(%d) to %d\n", e.from, info.label, e.to);
			} else if (rto == world_rank_cpu && rfrom == world_rank_cpu) {
				//assert(e.from >= offset);
				//assert(e.to >= offset);
        //printf();
        //printf();
				if (local_labels[e.from - offset] < local_labels[e.to - offset]) {
					local_labels[e.to - offset] = local_labels[e.from - offset];
					converged = 0;
				}
			}
		}
    printf("count %d\n", count);
		*((size_t*)ctrl_input_size) = count;

		*((uint32_t*)ctrl_input_valid) = 1;

    printf("CPU finishes 1 round of proc\n");
		// wait for fpga to have data ready to send
		while (!*((uint32_t*)ctrl_output_valid)) {
		//while (!*((uint32_t*)0x8002c328)) {
			;
		}
    printf("FPGA finishes 1 round of proc\n");
		printf("cpu: converged = %d\n", converged);

		if (*((uint32_t*)ctrl_converged) && converged) {
			printf("cpu: done = 1\n");
			*((uint32_t*)ctrl_done) = 1;
			*((uint32_t*)ctrl_output_valid) = 0;
			break;
		}

		size_t output_size = * ((size_t*)ctrl_output_size);
		printf("cpu: output_size = %lu\n", output_size);
		for (size_t i = 0; i < output_size; i++) {
			info_t info = output[i];
			//assert(info.label < MAX_VERTICES);
			//assert(info.label >= 0);

			if (local_labels[info.to - offset] > info.label)
				local_labels[info.to- offset] = info.label;
		}


		// tell fpga data has been sent
		printf("cpu: output_valid <- 0 (FPGA ctrl)\n");
		*((uint32_t*)ctrl_output_valid)= 0;

		// wait till send is done on the proc end
		while (*(uint32_t*)ctrl_input_valid) {
			;
		}

		printf("cpu: output_valid <- 0 (FPGA ctrl)\n");

	}
	printf("cpu: done\n");


//    while (1) {
//      // wait for fpga to have data ready to send
//      while (!ctrl->output_valid) {
//        ;
//      }
//
//      if (ctrl->converged) {
//
//        ctrl->done = 1;
//        printf("cpu: done\n");
//
//        // tell fpga data has been sent
//
//        ctrl->output_valid = 0;
//        break;
//      }
//
//      // tell fpga data has been sent
//      printf("cpu: output_valid <- 0\n");
//      ctrl->output_valid = 0;
//    }
//
//	  printf("cpu: done\n");
}

//#define MAX_EDGES_CPU 88234
#define MAX_EDGES_CPU 11
//#define MAX_VERTICES_CPU 4039 + 1024
#define MAX_VERTICES_CPU 11
static  info_t input[MAX_EDGES_CPU];
static  info_t output[MAX_EDGES_CPU];
static  label_t labels[MAX_VERTICES_CPU];

static  uint64_t from[MAX_EDGES_CPU] ={
  #include "edge_from_dummy.h"
  };
static  uint64_t to[MAX_EDGES_CPU] ={
  #include "edge_to_dummy.h"
  };

static  edge_t edges[MAX_EDGES_CPU  << 1];

int main() {

  size_t world_rank = 0;
  size_t world_size = 2;
  size_t num_edges = MAX_EDGES_CPU << 1;
  size_t num_vertices = MAX_VERTICES_CPU;

  for(int i = 0; i < num_edges  >> 1; i++){
    edges[i].from = from[i];
    edges[i].to = to[i];
  }
  for(int i = num_edges >> 1; i < num_edges; i++){
    edges[i].from = to[i- (num_edges >> 1)];
    edges[i].to = from[i- (num_edges >> 1)];
  }

  ctrl_t ctrl[1];

  printf("Call Top\n");

  uint64_t begin, end, dur;
  begin = read_cycle();

  //top_wrapper(&(ctrl->done), &(ctrl->converged), &(ctrl->input_valid), &(ctrl->output_valid), &(ctrl->output_size), &(ctrl->input_size), edges, input, output, labels, world_rank, world_size, num_edges, num_vertices, ctrl);

  graph (world_rank, world_size, num_vertices, num_edges, labels, edges);

  end = read_cycle();
  duration(begin, end);

  return 0;
}

