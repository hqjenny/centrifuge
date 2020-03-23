#include<stdio.h>
#include<stdint.h>
#include <stdlib.h>
#define LENGTH 80
#include "vadd_rocc.h"

void print_vec(int* vec, int length){
    for(int i = 0; i < length; i++){
        if (i != 0 ) printf(", ");
        printf("%d", vec[i]);
    }
}

int main () {
    // RoCC has a limited number of arguments so we must pack all the
    // information into two arrays:
    // length_a contains [length, a0, a1, ..., aN]
    // b_c contains [b0, ..., bN, c0, ..., cN]
    int length_a[LENGTH + 1], b_c[LENGTH + LENGTH];
    //int a[LENGTH], b[LENGTH], c[LENGTH];
    int length = LENGTH;
    length_a[0] = length;
    int i;
    for(i = 0; i < length; i++){
      length_a[i + 1] = i;
      b_c[i] = i + 5;
    }

    uint64_t begin, end, dur;
    vadd_rocc(length_a, b_c); 

    printf("A = [");
    print_vec(&length_a[1], length);
    printf("]\n");
        
    printf("B = [");
    print_vec(b_c, length);
    printf("]\n");
 
    printf("C = [");
    print_vec(&b_c[length], length);
    printf("]\n");

    // Verify:
    for(i = 0; i < length; i++) {
        if(b_c[length + i] != length_a[i + 1] + b_c[i]) {
            printf("Failure: Incorrect output at index %d:\n", i);
            printf("\tExpected: %d, Got: %d\n", (length_a[i + 1] + b_c[i]), b_c[length + i]);
            return EXIT_FAILURE;
        }
    }

    printf("Success!\n");
    return EXIT_SUCCESS;
}
