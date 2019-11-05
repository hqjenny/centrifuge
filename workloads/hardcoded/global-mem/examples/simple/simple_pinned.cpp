
#include <BCL.hpp>

void* pinned_malloc(size_t n_bytes) {
  return malloc(n_bytes);
}

int main(int argc, char** argv) {
  size_t n_bytes = 64*1024*1024;
  void* ptr = pinned_malloc(n_bytes);

  BCL::init(ptr, n_bytes);

  printf("Rank %d has ptr %p\n", BCL::rank(), ptr);

  BCL::finalize();
  return 0;
}
