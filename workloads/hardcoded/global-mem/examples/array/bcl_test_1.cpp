#include <BCL.hpp>

int main(int argc, char** argv) {
  BCL::init();
  printf("Hello! I'm %lu/%lu\n", BCL::rank(), BCL::nprocs());
  BCL::finalize();
  return 0;
}
