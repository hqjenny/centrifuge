#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include "graph.h"

#define NUM_NODES 1

struct thread_data {
	ctrl_t* ctrl;
	edge_t* edges;
	info_t* input;
	info_t* output;
	label_t* labels;
	int world_rank;
	int world_size;
	int num_edges;
	int num_vertices;
};

void test_helpers() {
}

void *top_wrapper(void *threadarg) {
	struct thread_data *my_data;
	my_data = (struct thread_data *) threadarg;
	top(my_data->ctrl, my_data->edges, my_data->input, my_data->output,
			my_data->labels, my_data->world_rank, my_data->world_size,
			my_data->num_edges, my_data->num_vertices);
	pthread_exit(NULL);
}

void *cpu_sim(void *threadarg) {
	struct thread_data *my_data = (struct thread_data *) threadarg;
	bool converged = true;
	int offset = rtov_lower(my_data->world_rank, my_data->world_size, my_data->num_vertices);
	int upper = rtov_upper(my_data->world_rank, my_data->world_size, my_data->num_vertices);
	int num_labels = upper - offset;
	int num_edges = my_data->num_edges;
	int world_size = my_data->world_size;
	int world_rank = my_data->world_rank;
	int num_vertices = my_data->num_vertices;
	info_t *input = my_data->input;
	edge_t *edges = my_data->edges;


	label_t local_labels[MAX_VERTICES/NUM_NODES/2];

	for (int i = 0; i < num_labels; i++) {
		//local_labels[i] = my_data->labels[i + offset];
		local_labels[i] = i + offset;
	}

	ctrl_t *ctrl = my_data->ctrl;

  int iter = 0;
	while (1) {

		converged = true;
		int count = 0;
		for (int i = 0; i < num_edges; i++) {
			edge_t e = edges[i];

			int rto = vtor(e.to, world_size, num_vertices);
			int rfrom = vtor(e.from, world_size, num_vertices);
      //printf("rfrom%d(%d) rto%d(%d)\n", rfrom, e.from, rto, e.to);
			if (rto != world_rank && rfrom == world_rank) {
				info_t info;
				info.to = e.to;
				info.label = local_labels[e.from - offset];
				input[count++] = info;
        printf("cpu: send from %d(%d) to %d\n", e.from, info.label, e.to);
			} else if (rto == world_rank && rfrom == world_rank) {
				assert(e.from >= offset);
				assert(e.to >= offset);

        printf("cpu: update from %d(%d) to %d(%d)\n", e.to, local_labels[e.to - offset], e.from, local_labels[e.from - offset]);
				if (local_labels[e.from - offset] < local_labels[e.to - offset]) {

					local_labels[e.to - offset] = local_labels[e.from - offset];
					converged = false;
				}
			}
		}
		ctrl->input_size = count;

		ctrl->input_valid = true;

		// wait for fpga to have data ready to send
		while (!ctrl->output_valid) {
			;
		}

		printf("cpu: converged = %i\n", converged);

		//if (ctrl->converged && converged && iter !=0 ) {
		if (ctrl->converged && converged) {
			printf("cpu: done <- true\n");
			ctrl->done = 1;
			ctrl->output_valid = 0;
			break;
		}

		size_t output_size = ctrl->output_size;
		printf("cpu: output_size = %lu\n", output_size);
		for (size_t i = 0; i < output_size; i++) {
			info_t info = my_data->output[i];
			assert(info.label < MAX_VERTICES);
			assert(info.label >= 0);

      printf("cpu: recv to %d label %d\n", info.to, info.label);
			if (local_labels[info.to - offset] > info.label) {
				local_labels[info.to - offset] = info.label;
      }
		}

		// tell fpga data has been sent
		printf("cpu: output_valid <- false (FPGA ctrl)\n");
		ctrl->output_valid = 0;
    
		while (ctrl->input_valid) {
			;
		}

    iter++;
	}

  for (int i = offset;
      i < upper; i++) {
      my_data->labels[i] = local_labels[i - offset];
  }
  printf("offset %d ", offset);
  printf("upper %d\n", rtov_upper(world_rank, world_size, num_vertices));


	printf("cpu: done\n");
	pthread_exit(NULL);
}

edge_t edges[MAX_EDGES];
size_t num_edges;
info_t input[MAX_EDGES];
info_t output[MAX_EDGES];
label_t labels[MAX_VERTICES];

/**
 * Simulates CPU-Accelerator coprocessing system
 * Each CPU/Accelerator is a pthread
 * threads[0]: cpu 0
 * threads[1]: accel 0
 */

int main(int argc, char *argv[]) {
	const char *filename;
	pthread_t threads[2 * NUM_NODES];
	struct thread_data my_data[2 * NUM_NODES];
	ctrl_t ctrl[NUM_NODES];

	bool undirected = false;

  for(int i; i < MAX_VERTICES; i++) {
    labels[i]=i;
  }
	int opt;
	while ((opt = getopt(argc, argv, "u")) != -1)
	{
		switch (opt) {
			case 'u':
				undirected = true;
				break;
			default:
				fprintf(stderr, "Usage: %s [-u] edgelist\n", argv[0]);
				exit(1);
		}
	}
	if (optind + 1 != argc) {
		fprintf(stderr, "Usage: %s [-u] edgelist\n", argv[0]);
		exit(1);
	}

	filename = argv[optind];


	FILE *file = fopen(filename, "r");

	if (file == NULL)
	{
		fprintf(stderr, "Failed to open graph file\n");
		exit(1);
	}

	num_edges = 0;
	size_t num_vertices = 0;

	while (!feof(file)) {
		if (num_edges >= MAX_EDGES) {
			fprintf(stderr, "Graph has too many edges: more than num_edges %lu\n", num_edges);
			exit(1);
		}
		edge_t edge;
		fscanf(file, "%lu %lu\n", &edge.from, &edge.to);
		if (edge.from > num_vertices)
			num_vertices = edge.from;
		if (edge.to  > num_vertices) {
			num_vertices = edge.to;
		}
		edges[num_edges++] = edge;
		if (undirected && num_edges < MAX_EDGES) {
			edges[num_edges].from = edge.to;
			edges[num_edges++].to = edge.from;
		}
	}

	// +1 because vertex ID starts at 0
	num_vertices += 1;

	if (num_vertices >= MAX_VERTICES) {
		fprintf(stderr, "Too many vertices: %lu\n", num_vertices);
		exit(1);
	}

	printf("num_edges = %lu\n", num_edges);
	printf("num_vertices = %lu\n", num_vertices);

	assert(num_edges > MAX_VERTICES);

	my_data[0].ctrl = &ctrl[0];
	my_data[0].edges = edges;
	my_data[0].input = input;
	my_data[0].output = output;
	my_data[0].labels = labels;
	my_data[0].world_rank = 0;
	my_data[0].world_size = 2;
	my_data[0].num_edges = num_edges;
	my_data[0].num_vertices = num_vertices;

	my_data[1].ctrl = &ctrl[0];
	my_data[1].edges = edges;
	my_data[1].input = input;
	my_data[1].output = output;
	my_data[1].labels = labels;
	my_data[1].world_rank = 1;
	my_data[1].world_size = 2;
	my_data[1].num_edges = num_edges;
	my_data[1].num_vertices = num_vertices;

	ctrl[0].input_size = 0;
	ctrl[0].output_size = 0;
	ctrl[0].converged = 0;
	ctrl[0].done = 0;
	ctrl[0].input_valid = 0;
	ctrl[0].output_valid = 0;

	pthread_create(&threads[0], NULL, cpu_sim, (void *) &my_data[0]);
	pthread_create(&threads[1], NULL, top_wrapper, (void *) &my_data[1]);

	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);

	size_t num_partitions = 0;
	for (size_t i = 0; i < num_vertices; i++) {
     //printf("%d ", labels[i]);
		if (labels[i] == i) {
			num_partitions++;
			printf("%lu\n", i);
		}
	}
	printf("num_partitions: %lu\n", num_partitions);
}
