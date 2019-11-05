#include <cstdlib>
#include <cstdio>
#include <string>

#include <BCL.hpp>
#include <containers/Array.hpp>

int main(int argc, char **argv) {
  BCL::init();

  printf("Hi, I'm rank %d/%d\n", BCL::rank(), BCL::nprocs());

  int fill = 10;
  BCL::Array <int> array(0, BCL::nprocs() * fill);

  std::vector <int> my_array;
  int j = 0;
  for (int i = BCL::rank() * fill; i < (BCL::rank() + 1) * fill; i++) {
    my_array.push_back(BCL::rank() + j++);
    // array[i] = BCL::rank() + j++;
  }

  printf("Pushing vector of size %d to %d/%d\n", my_array.size(), BCL::rank() * fill,
    array.size());

  array.put(BCL::rank() * fill, my_array);

  BCL::barrier();

  if (BCL::rank() == 0) {
    for (int i = 0; i < array.size(); i++) {
      printf("%d ", *array[i]);
    }
    printf("\n");
  }

  BCL::finalize();
  return 0;
}
