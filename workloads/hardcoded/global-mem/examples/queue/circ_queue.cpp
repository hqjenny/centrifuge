#include <cstdlib>
#include <cstdio>

#include "../../BCL.hpp"
#include "../../containers/CircularQueue.hpp"

void insert_bunch(BCL::CircularQueue <std::string> &queue) {
  for (int i = 0; i < 100; i++) {
    std::string val = "I, rank " + std::to_string(BCL::rank()) + " am pushing " + std::to_string(i);
    bool success = queue.push(val);
  }
}

int pop_all(BCL::CircularQueue <std::string> &queue) {
  bool success;

  int popped = 0;

  do {
    std::string val;
    success = queue.pop(val);
    if (success) {
      popped++;
    }
  } while (success);
  return popped;
}

int main(int argc, char **argv) {
  BCL::init(256, true);
  BCL::CircularQueue <std::string> queue(0, 100);

  for (int host = 0; host < BCL::nprocs(); host++) {
    queue.migrate(host);
    if (BCL::rank() == 0) {
      printf("INSERT PHASE %d\n", host);
      fflush(stdout);
    }
    BCL::barrier();
    insert_bunch(queue);
    BCL::barrier();
    if (BCL::rank() == 0) {
      printf("  %d in queue\n", queue.size());
      fflush(stdout);
    }
    queue.resize(queue.capacity()+100);
  }

  int popped = pop_all(queue);

  int total_popped = BCL::allreduce(popped, BCL::plus <int> ());

  if (BCL::rank() == 0) {
    printf("%d total popped\n", total_popped);
  }

  BCL::finalize();
  return 0;
}
