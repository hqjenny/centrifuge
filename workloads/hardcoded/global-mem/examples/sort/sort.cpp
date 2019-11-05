#include <cstdlib>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <chrono>
#include <stdexcept>

#include "../../BCL.hpp"

#include "../../containers/CircularQueue.hpp"

/*
template <typename T>
struct FixedQueue {
  BCL::GlobalPtr <T> data = nullptr;
  BCL::GlobalPtr <int> tail = nullptr;

  size_t my_capacity = 0;
  uint64_t my_host = 0;

  FixedQueue(const size_t capacity, const uint64_t host) :
    my_capacity(capacity), my_host(host) {
    if (BCL::rank() == this->host()) {
      data = BCL::alloc <T> (this->capacity());
      tail = BCL::alloc <int> (1);

      if (data == nullptr || tail == nullptr) {
        throw std::runtime_error("FixedQueue: ran out of memory.");
      }

      for (int i = 0; i < this->capacity(); i++) {
        new (&data.local()[i]) T();
      }
      *tail.local() = 0;
    }

    data = BCL::broadcast(data, this->host());
    tail = BCL::broadcast(tail, this->host());
  }

  bool push(const T &val) {
    int my_loc = BCL::fetch_and_op <int> (tail, 1, BCL::plus <int> ());

    if (my_loc < capacity()) {
      BCL::rput(val, data + my_loc);
      return true;
    } else {
      BCL::fetch_and_op <int> (tail, -1, BCL::plus <int> ());
      return false;
    }
  }

  bool push(const std::vector <T> &vals) {
    if (vals.size() == 0) {
      return true;
    }
    int my_loc = BCL::fetch_and_op <int> (tail, vals.size(), BCL::plus <int> ());

    if (my_loc + (vals.size()-1) < capacity()) {
      BCL::rput(vals.data(), data + my_loc, vals.size());
      return true;
    } else {
      BCL::fetch_and_op <int> (tail, -vals.size(), BCL::plus <int> ());
      return false;
    }
  }

  size_t capacity() const noexcept {
    return my_capacity;
  }

  size_t size() const {
    return BCL::rget(tail);
  }

  uint64_t host() const noexcept {
    return my_host;
  }
};
*/

int main (int argc, char **argv) {
  BCL::init(1024);

  // Sort 1M integers
  const size_t n_vals = 1024*1024*1;
  const size_t bucket_size = n_vals*2;

  // Generate random values between [0, 1G)
  const int range = 1000000000;

  // Tuning parameter
  const int message_size = 1024*1;

  std::vector <BCL::CircularQueue <int>> queues;

  for (int rank = 0; rank < BCL::nprocs(); rank++) {
    queues.push_back(BCL::CircularQueue <int> (rank, bucket_size));
  }

  srand48(BCL::rank());

  std::vector <int> vals;

  for (int i = 0; i < n_vals; i++) {
    vals.push_back(lrand48() % range);
  }

  BCL::barrier();
  auto begin = std::chrono::high_resolution_clock::now();

  std::vector <std::vector <int>> message_queues(BCL::nprocs());

  int partition_size = (range + BCL::nprocs() - 1) / BCL::nprocs();

  for (const auto &val : vals) {
    int my_node = val / partition_size;
    message_queues[my_node].push_back(val);
    if (message_queues[my_node].size() >= message_size) {
      bool success = queues[my_node].push(message_queues[my_node]);
      if (!success) {
        throw std::runtime_error("error: Queue on " + std::to_string(my_node) +
          "out of space");
      }
      message_queues[my_node].clear();
    }
  }

  for (int rank = 0; rank < message_queues.size(); rank++) {
    queues[rank].push(message_queues[rank]);
    message_queues[rank].clear();
  }

  BCL::barrier();
  auto end_insert = std::chrono::high_resolution_clock::now();

  double insert = std::chrono::duration <double> (end_insert - begin).count();
  /*
  if (BCL::rank() == 0) {
    printf("%lf inserting\n", insert);
    fflush(stdout);
  }
  */
  BCL::barrier();
  auto begin_local_sort = std::chrono::high_resolution_clock::now();

  std::vector <int> my_bucket = queues[BCL::rank()].as_vector();

  std::sort(my_bucket.begin(), my_bucket.end());

  BCL::barrier();
  auto end = std::chrono::high_resolution_clock::now();

  double local_sort = std::chrono::duration <double> (end - begin_local_sort).count();
  double total = insert + local_sort;

  if (BCL::rank() == 0) {
    printf("%lf total, %lf insert, %lf local sort.\n", total, insert, local_sort);
  }

/*
  for (int rank = 0; rank < BCL::nprocs(); rank++) {
    if (BCL::rank() == rank) {
      printf("Rank %d has: \n", rank);
      for (const auto &val : my_bucket) {
        std::cout << val << std::endl;
      }
    }
    fflush(stdout);
    BCL::barrier();
  }
  */

  BCL::finalize();
  return 0;
}
