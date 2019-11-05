
#include <vector>

#include <BCL.hpp>

#define TIMEIT(code, lbl) \
        auto begin_##lbl = std::chrono::high_resolution_clock::now(); \
        code \
        auto end_##lbl = std::chrono::high_resolution_clock::now(); \
        double duration_##lbl = std::chrono::duration<double>(end_##lbl - begin_##lbl).count(); \
        BCL::print(#lbl ", %lfs.\n", duration_##lbl);


int main(int argc, char **argv) {
  BCL::init(1024);
  std::vector<BCL::GlobalPtr<int>> ptrs(BCL::nprocs());

  size_t n = 128*1024*1024;

  for (size_t i = 0; i < BCL::nprocs(); i++) {
    if (BCL::rank() == i) {
      ptrs[i] = BCL::alloc<int>(n);
    }
    ptrs[i] = BCL::broadcast(ptrs[i], i);
  }

  for (size_t i = 0; i < n; i++) {
    ptrs[BCL::rank()].local()[i] = BCL::rank();
  }

  std::vector<BCL::future<std::vector<int>>> vec_futures(BCL::nprocs());
  std::vector<std::vector<int>> vecs(BCL::nprocs());
  BCL::barrier();

  auto begin = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < BCL::nprocs(); i++) {
    vec_futures[i] = std::move(BCL::arget(ptrs[i], n));
  }
  auto end = std::chrono::high_resolution_clock::now();
  double duration = std::chrono::duration<double>(end - begin).count();

  printf("%lf async issues...\n", duration);

  // MPI_Barrier(MPI_COMM_WORLD);
  begin = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < BCL::nprocs(); i++) {
    vecs[i] = vec_futures[i].get();
  }
  end = std::chrono::high_resolution_clock::now();
  duration = std::chrono::duration<double>(end - begin).count();
  printf("%lf async waits...\n", duration);
  BCL::barrier();

  BCL::finalize();

  return 0;
}
