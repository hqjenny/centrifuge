#include <cstdlib>
#include <cstdio>

#include "../../BCL.hpp"
#include "../../containers/QueueList.hpp"

int main(int argc, char **argv) {
  BCL::init(256, true);
  BCL::QueueList <std::string> queue(0);

  for (int i = 0; i < 10; i++) {
    queue.insert(std::string("I, node ") + std::to_string(BCL::rank()) +
      "am inserting " + std::to_string(i));
  }

  BCL::finalize();
  return 0;
}
