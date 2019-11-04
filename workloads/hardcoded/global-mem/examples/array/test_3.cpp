#include <mpi.h>
#include <unistd.h>
#include <string>
#include <cstdio>

size_t rank() {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  return rank;
}

size_t nprocs() {
  int nprocs;
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  return nprocs;
}

std::string hostname() {
  char hostname[2048];
  gethostname(hostname, 2048);
  return std::string(hostname, 2048);
}

template <typename T>
T broadcast(T& value, size_t root) {
  MPI_Bcast(&value, sizeof(T), MPI_CHAR, root, 
            MPI_COMM_WORLD);
  return value;
}

int main(int argc, char** argv) {
  MPI_Init(NULL, NULL);
  printf("Hello! I am %lu/%lu on %s\n",
         rank(), nprocs(), hostname().c_str());
  printf("%lu Trying barrier...\n", rank());
  fflush(stdout);
  MPI_Barrier(MPI_COMM_WORLD);
  printf("%lu After barrier...\n", rank());
  fflush(stdout);
  size_t value = 12;
  value = broadcast(value, 0);
  printf("%lu got value %lu\n", rank(), value);
  fflush(stdout);
  MPI_Finalize();
  return 0;
}
