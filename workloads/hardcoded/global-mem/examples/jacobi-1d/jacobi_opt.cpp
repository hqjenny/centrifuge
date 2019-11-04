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
  try {
    array[proc][proc_slot] = val;
  } catch (const std::exception &e) {
    printf("Hey!\n");
  }
}

float aget(size_t idx, std::vector <BCL::Array <float>> &array,
  size_t segment_size) {
  size_t proc = idx / segment_size;
  size_t proc_slot = idx - proc*segment_size;
  float rv;
  try {
    rv = *array[proc][proc_slot];
  } catch (const std::exception &e) {
    printf("Hey!\n");
    return rv;
  }
}

int main(int argc, char **argv) {
  BCL::init(1024);
  size_t sim_size = 1*1024*1024*1024;
  size_t n_iter = 100;
  float z = 4.0f;

  size_t segment_size = (sim_size + BCL::nprocs() - 1) / BCL::nprocs();
  std::vector <BCL::Array <float>> array_read;
  for (size_t i = 0; i < BCL::nprocs(); i++) {
    array_read.push_back(BCL::Array <float> (i, segment_size));
  }

  std::vector <BCL::Array <float>> array_write;
  for (size_t i = 0; i < BCL::nprocs(); i++) {
    array_write.push_back(BCL::Array <float> (i, segment_size));
  }
  size_t my_begin = segment_size * BCL::rank();
  size_t my_end = std::min(my_begin + segment_size, sim_size);

  if (BCL::rank() == 0) {
    awrite(1000.0f, 0, array_read, segment_size);
    for (size_t i = 1; i < my_end; i++) {
      awrite(0.0f, i, array_read, segment_size);
    }
  } else if (BCL::rank() == BCL::nprocs()-1) {
    awrite(1000.0f, sim_size-1, array_read, segment_size);
    for (size_t i = my_begin; i < my_end-1; i++) {
      awrite(0.0f, i, array_read, segment_size);
    }
  } else {
    for (size_t i = my_begin; i < my_end; i++) {
      awrite(0.0f, i, array_read, segment_size);
    }
  }

  size_t my_offset = my_begin;

  if (BCL::rank() == 0) {
    my_begin++;
  }
  if (BCL::rank() == BCL::nprocs()-1) {
    my_end--;
  }

  BCL::barrier();
  auto begin = std::chrono::high_resolution_clock::now();

  float *__restrict__ local_array = (float *) __builtin_assume_aligned(decay_container(array_read[BCL::rank()].data).local() - my_offset,
      8);

  for (size_t i = 0; i < n_iter; i++) {
    float left = aget(my_begin-1, array_read, segment_size);
    float xbf = 2.0f*left;
    float xat = (1.0f - 2.0f*z) * local_array[my_begin];
    float xaf = 2.0f * local_array[my_begin + 1];
    float me = xbf + xat + xaf;
    local_array[my_begin] = xbf + xat + xaf;
    for (size_t j = my_begin+1; j < my_end-1; j++) {
      float xbf = 1.0f*local_array[j-1];
      float xat = (1.0f - 2.0f*z)*local_array[j];
      float xaf = 2.0f*local_array[j+1];
      float me = xbf + xat + xaf;
      local_array[j] = me;
    }
    float right = aget(my_end, array_read, segment_size);
    xbf = 2.0f*local_array[my_end-2];
    xat = (1.0f - 2.0f*z)*local_array[my_end-1];
    xaf = 2.0f*right;
    me = xbf + xat + xaf;
    local_array[my_end - 1] = me;
    std::swap(array_write, array_read);
    BCL::barrier();
  }
  auto end = std::chrono::high_resolution_clock::now();

  double elapsed = std::chrono::duration <double> (end - begin).count();

  float res = 0.0f;
  /*
  for (int i = 0; i < array_write.size(); i++) {
    for (int j = 0; j < segment_size; i++) {
      res += array_write[i][j].get();
    }
  }
  */

  BCL::print("Took %lf s (%lf/iteration) %lf\n", elapsed, elapsed / n_iter, res);

  BCL::finalize();
  return 0;
}
