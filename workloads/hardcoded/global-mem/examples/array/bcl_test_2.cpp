#include <BCL.hpp>

int main(int argc, char** argv) {
  BCL::init();
  printf("Hello! I'm %lu/%lu\n", BCL::rank(), BCL::nprocs());
  size_t value = BCL::rank();
  value = BCL::broadcast(value, 0);
  BCL::barrier();
  value = BCL::broadcast(value, 0);

  printf("%lu after!\n", BCL::rank());
  BCL::finalize();
  return 0;
}
