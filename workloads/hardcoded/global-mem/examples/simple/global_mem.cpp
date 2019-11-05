#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <cmath>

#include <BCL.hpp>
#include <containers/HashMap.hpp>

int main(int argc, char **argv) {
  BCL::init();

  BCL::HashMap <int, int> map(1000);

  bool success = map.insert(BCL::rank(), BCL::rank()*10);

  BCL::barrier();

  for (int i = 0; i < BCL::nprocs(); i++) {
    int rv;
    bool success = map.find(i, rv);
    if (success == false) {
      throw std::runtime_error("AGH!\n");
    }
    printf("%d: %d\n", i, rv);
  }

  BCL::finalize();
  return 0;
}
