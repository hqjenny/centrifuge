#include <cstdlib>
#include <cstdio>
#include <vector>
#include <chrono>
#include <BCL.hpp>

#include <containers/Array.hpp>

void awrite(float val, size_t idx, std::vector <BCL::Array <float>> &array,
  size_t segment_size) {
  size_t proc = idx / segment_size;
  size_t proc_slot = idx - proc*segment_size;
  array[proc][proc_slot] = val;
}

float aget(size_t idx, std::vector <BCL::Array <float>> &array,
  size_t segment_size) {
  size_t proc = idx / segment_size;
  size_t proc_slot = idx - proc*segment_size;
  return *array[proc][proc_slot];
}

int main(int argc, char **argv) {
  BCL::init(1024);
  size_t sim_size = 64*1024*1024;
  size_t n_iter = 10;
  float z = 4.0f;

  size_t segment_size = (sim_size + BCL::nprocs() - 1) / BCL::nprocs();
  std::vector <BCL::Array <float>> segments_read;
  for (size_t i = 0; i < BCL::nprocs(); i++) {
    segments_read.push_back(BCL::Array <float> (i, segment_size));
  }

  std::vector <BCL::Array <float>> segments_write;
  for (size_t i = 0; i < BCL::nprocs(); i++) {
    segments_write.push_back(BCL::Array <float> (i, segment_size));
  }
  size_t my_begin = segment_size * BCL::rank();
  size_t my_end = std::min(my_begin + segment_size, sim_size);

  if (BCL::rank() == 0) {
    awrite(1000.0f, 0, segments_read, segment_size);
    for (size_t i = 1; i < my_end; i++) {
      awrite(0.0f, i, segments_read, segment_size);
    }
  } else if (BCL::rank() == BCL::nprocs()-1) {
    awrite(1000.0f, sim_size-1, segments_read, segment_size);
    for (size_t i = my_begin; i < my_end-1; i++) {
      awrite(0.0f, i, segments_read, segment_size);
    }
  } else {
    for (size_t i = my_begin; i < my_end; i++) {
      awrite(0.0f, i, segments_read, segment_size);
    }
  }

  BCL::barrier();
  auto begin = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < n_iter; i++) {
    for (size_t j = my_begin; j < my_end; j++) {
      if (j != 0 && j != sim_size-1) {
        float xbf = 2.0f*aget(j-1, segments_read, segment_size);
        float xat = (1.0f - 2.0f*z)*aget(j, segments_read, segment_size);
        float xaf = 2.0f*aget(j+1, segments_read, segment_size);
        float me = xbf + xat + xaf;

        awrite(me, j, segments_write, segment_size);
      }
    }
    std::swap(segments_write, segments_read);
    BCL::barrier();
  }
  auto end = std::chrono::high_resolution_clock::now();

  double elapsed = std::chrono::duration <double> (end - begin).count();

  BCL::print("Took %lf s (%lf/iteration)\n", elapsed, elapsed / n_iter);

  BCL::finalize();
  return 0;
}
