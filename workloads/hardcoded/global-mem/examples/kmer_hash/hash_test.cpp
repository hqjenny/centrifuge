#include <cstdio>
#include <cstdlib>

#include "../../BCL.hpp"
#include "../../containers/HashMap.hpp"

#include "../../containers/HashMapBuffer.hpp"

int main(int argc, char **argv) {
  BCL::init();

  BCL::HashMap <std::string, int> map(1000);

  BCL::HashMapBuffer <std::string, int> buffer(map, 100, 10);

  for (int i = 0; i < 10; i++) {
    buffer.insert(std::to_string(i) + std::to_string(BCL::rank()), i + BCL::rank());
  }

  buffer.flush();

  if (BCL::rank() == 0) {
    for (int rank = 0; rank < BCL::nprocs(); rank++) {
      for (int i = 0; i < 10; i++) {
        int val = map.find(std::to_string(i) + std::to_string(BCL::rank()));
        printf("Got %s: %d\n", (std::to_string(i) + std::to_string(BCL::rank())).c_str(),
          val);
      }
    }
  }

  printf("Done inserting hashvals...\n");

  BCL::finalize();
  return 0;
}
