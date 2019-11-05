
// Doin' things with sparse matrices...

#include <cmath>
#include <cassert>
#include <iostream>

#include <chrono>

#include <BCL.hpp>
#include <containers/DMatrix.hpp>

//#include <cblas.h>

#include "summa.hpp"
#include "hls_driver.c"

void* pinned_malloc(size_t size) {

  int fd =  mmap_init();
  void* kernel_buf = (void*) hls_kalloc(fd, size);

  puts("init memory\n");
  return kernel_buf; 
}

template <typename T>
bool allclose(std::vector<T> a, std::vector<T> b, T rtole = 1e-05, T atole = 1e-08);
double num_gflops(size_t M, size_t N, size_t K);

int main(int argc, char **argv) {
	if (argc < 4) {
		printf("Please specify input args!\n");	
		return 0;	
	}
	size_t tile_size = atoi(argv[1]);
	size_t mat_size = atoi(argv[2]);
	unsigned char accel = atoi(argv[3]);

  size_t M = mat_size;
  size_t N = mat_size;
  size_t K = mat_size;

  //BCL::init(1024);
  size_t grid_dim = sqrt(BCL::nprocs());
//  size_t tile_size = 512;
//  size_t M = 4096;
//  size_t N = 4096;
//  size_t K = 4096;
//  size_t tile_size = 16;
//  size_t M = 64;
//  size_t N = 64;
//  size_t K = 64;
// size_t tile_size = 256;
//  size_t M = 256;
//  size_t N = 256;
//  size_t K = 256;

  printf("tile_size = %d mat_size = %d\n", tile_size, mat_size);
  printf("is accel? %d\n", accel);
  using T = short;
  size_t n_bytes = tile_size * tile_size * 64 * sizeof(T);
  void* ptr = pinned_malloc(n_bytes);
  assert(ptr);
  BCL::init(ptr, n_bytes);
  printf("Rank %d has ptr %p\n", BCL::rank(), ptr);
//  BCL::init();

  BCL::DMatrix<T> a(M, K, grid_dim, grid_dim, tile_size, tile_size);
  BCL::DMatrix<T> b(K, N, grid_dim, grid_dim, tile_size, tile_size);
  BCL::DMatrix<T> c(M, N, grid_dim, grid_dim, tile_size, tile_size);
  BCL::DMatrix<T> c_ref(M, N, grid_dim, grid_dim, tile_size, tile_size);

  // BCL init 
  //BCL::fill_range(a, 10);
  //BCL::fill_range(b, 9);
  //a = 10;
  //b = 9;
  //auto a_print = a.get_matrix();
  //auto b_print = b.get_matrix();

  //for (int i= 0; i < 10; i ++) {
  //  a_print[i] = i + 10;
  //  b_print[i << 8] = i + 10;
  //}

  //c = 0;
  //c_ref = 0;

  BCL::barrier();
  BCL::print("Beginning SUMMA...\n");
  BCL::barrier();
  auto begin = std::chrono::high_resolution_clock::now();
  if (accel == 0) {
    BCL::experimental::gemm(BCL::experimental::BlasTrans::notrans,
                          BCL::experimental::BlasTrans::notrans,
                          a, b, c, BCL::experimental::cpu);
  } else if (accel == 1) {
    BCL::experimental::gemm(BCL::experimental::BlasTrans::notrans,
                         BCL::experimental::BlasTrans::notrans,
                         a, b, c_ref, BCL::experimental::accel);
  } else{
	;	
  }
  auto load_begin = std::chrono::high_resolution_clock::now();
  BCL::barrier();
  auto end = std::chrono::high_resolution_clock::now();
  auto load_end = end;

  auto load_imbalance = std::chrono::duration<double>(load_end - load_begin).count();

  // auto c_print = c_ref.get_matrix();
  double d_duration = std::chrono::duration<double>(end - begin).count();

  BCL::print("Distributed SUMMA got %lf GFLOPs (elapsed %lf s)\n",
             num_gflops(c.m(), c_ref.n(), a.n()) / d_duration, d_duration);

  BCL::print("%lf compute, %lf issue, %lf sync, %lf load imbalance (seconds)\n",
             BCL::experimental::compute,
             BCL::experimental::issue,
             BCL::experimental::sync,
             BCL::experimental::load_imbalance);

  double total = BCL::experimental::compute +
                 BCL::experimental::issue + 
                 BCL::experimental::sync + 
                 BCL::experimental::load_imbalance;

  BCL::print("%lf%% compute, %lf%% issue, %lf%% sync, %lf%% load imbalance\n",
             100*(BCL::experimental::compute / total),
             100*(BCL::experimental::issue / total),
             100*(BCL::experimental::sync / total),
             100*(BCL::experimental::load_imbalance / total));

  BCL::print("%lf%% compute, %lf%% communication, %lf%% load imbalance\n",
             100*(BCL::experimental::compute / total),
             100*((BCL::experimental::compute + BCL::experimental::sync) / total),
             100*(BCL::experimental::load_imbalance / total));

//  BCL::barrier();
//  BCL::print("Beginning SUMMA Accelerator...\n");
//  BCL::barrier();
//  begin = std::chrono::high_resolution_clock::now();
//  BCL::experimental::gemm(BCL::experimental::BlasTrans::notrans,
//                         BCL::experimental::BlasTrans::notrans,
//                         a, b, c_ref, BCL::experimental::accel);
//
//  BCL::barrier();
//  end = std::chrono::high_resolution_clock::now();
//  d_duration = std::chrono::duration<double>(end - begin).count();
//
//  BCL::print("Distributed SUMMA Accelerator got %lf GFLOPs (elapsed %lf s)\n",
//             num_gflops(c.m(), c.n(), a.n()) / d_duration, d_duration);
//
//
//  for (int i= 0; i < 10; i ++) {
//    printf("a[%d]%d \t",i, a_print[i]);
//    printf("b[%d]%d \t",i, b_print[i]);
//    printf("c[%d]%d \t",i, c_print[i]);
//    printf("\n");
//  }
  //  bool correct = allclose(c.get_matrix(), c_ref.get_matrix());

  //  if (correct) {
  //    printf("W00t! We're correct.\n");
  //}

  BCL::finalize();
  return 0;
}

template <typename T>
bool allclose(std::vector<T> a, std::vector<T> b, T rtole, T atole) {
  assert(a.size() == b.size());

  T total_diff = 0.0f;
  T max_diff = 0.0f;
  size_t num_off = 0;
  for (size_t i = 0; i < a.size(); i++) {
    if (std::abs(a[i] - b[i]) > atole + rtole * std::abs(b[i])) {
      num_off++;
      max_diff = std::max(max_diff, (short)std::abs(a[i] - b[i]));
      total_diff += std::abs(a[i] - b[i]);
    }
  }

  if (num_off > 0) {
    printf("not allclose: {%f,%f,%f,%lu} {max,avg,total,num_off}\n",
           max_diff, total_diff / num_off, total_diff, num_off);
    return false;
  } else {
    return true;
  }
}

double num_gflops(size_t M, size_t N, size_t K) {
  return 1e-9 * (2*M*N*K + 3*M*N);
}
