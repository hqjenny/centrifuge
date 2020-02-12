#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#define LENGTH 80
#include "vadd_rocc.h"
#include "vadd_tl.h"

// Manually calculated reference output
int golden_output[LENGTH + 1] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79};

void print_vec(int* vec, int length){
    for(int i = 0; i < length; i++){
        if (i != 0 ) printf(", ");
        printf("%d", vec[i]);
    }
}

// Check whether two vectors are equal. Returns the first index at
// which they differ, or -1 if they are identical.
int compare_vec(int *a, int *b, int length) {
  for(int i = 0; i < length; i++) {
    if(a[i] != b[i]) {
      return i;
    }
  }
  return -1;
}

int test_rocc(int *c, int *a, int *b, int len) {
    int length_a[len + 1], b_c[len + len];
    length_a[0] = len;

    // Copy inputs into rocc-formatted arrays
    for(int i = 0; i < len; i++){
      length_a[i + 1] = a[i];
      b_c[i] = b[i];
    }

    uint64_t begin, end, dur;
    vadd_rocc(length_a, b_c); 

    // Get output into form expected by test
    for(int i = 0; i < len; i++){
      c[i] = b_c[len + i];
    }

    return 0;
}

int test_tl(int *c, int *a, int *b, int len) {
  vadd_tl(a, b, c, len);
  return 0;
}

int main(int argc, char *argv[]) {
    int a[LENGTH], b[LENGTH];
    int rocc_out[LENGTH] = {0};
    int tl_out[LENGTH] = {0};
    int golden_out[LENGTH] = {0};

    for(int i = 0; i < LENGTH; i++){
      a[i] = i;
      b[i] = i + 5;
    }

    for(int i = 0; i < LENGTH; i++) {
      golden_out[i] = a[i] + b[i];
    }

    test_rocc(rocc_out, a, b, LENGTH);
    if(compare_vec(rocc_out, golden_out, LENGTH) != -1) {
      printf("Test Failure: rocc output does not match\n");
      printf("Expected:\n");
      print_vec(golden_out, LENGTH);
      printf("Got:\n");
      print_vec(rocc_out, LENGTH);
      return EXIT_FAILURE;
    }
      
    test_tl(tl_out, a, b, LENGTH);
    if(compare_vec(tl_out, golden_out, LENGTH) != -1) {
      printf("Test Failure: tilelink output does not match\n");
      printf("Expected:\n");
      print_vec(golden_out, LENGTH);
      printf("Got:\n");
      print_vec(tl_out, LENGTH);
      return EXIT_FAILURE;
    }

    printf("Test Success!\n");
    return EXIT_SUCCESS;
}
