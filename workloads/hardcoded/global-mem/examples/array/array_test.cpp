#include "../../BCL.hpp"

#include "array.hpp"

int main(int argc, char **argv) {
  BCL::init();
  Array <int> f(0, 4, 4);

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      f(i, j) = i + j;
    }
  }

  printf("Main array is\n");
  f.print();

  printf("Slice is\n");
  ArrayView <int> view = f(slc(1, 3), slc(1, 3));
  view.print();

  printf("view = 1\n");
  view = 1;
  view.print();

  printf("Main array is now\n");
  f.print();

  ArrayView <int> view2 = f(slc(2, 4), slc(2, 4));

  printf("Second view is\n");
  view2.print();

  view = view2;

  printf("First view is now\n");
  view.print();

  printf("Main array is now\n");
  f.print();

  printf("Okay! Now let's realize a view.\n");

  ArrayView <int> my_view = f(slc(0, 2), slc(0, 2));

  Array <int> realized_view(0, 2, 2);

  printf("I picked view\n");
  my_view.print();

  realized_view(slc(0, 2), slc(0, 2)) = my_view;

  printf("Realized it as\n");

  realized_view.print();


  realized_view(slc(0, 2), slc(0, 2)) = -12;
  printf("Now setting to\n");

  realized_view.print();

  printf("Original is now\n");

  f.print();

  BCL::finalize();
  return 0;
}
