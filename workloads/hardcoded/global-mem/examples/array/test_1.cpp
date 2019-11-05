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

int main(int argc, char** argv) {
  MPI_Init(NULL, NULL);
  printf("Hello! I am %lu/%lu on %s\n",
         rank(), nprocs(), hostname().c_str());
  MPI_Finalize();
  return 0;
}
