#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#define LENGTH 80
#include "vadd_tl.h"

void print_vec(int* vec, int length){
    for(int i = 0; i < length; i++){
        if (i != 0 ) printf(", ");
        printf("%d", vec[i]);
    }
}

int main () {
    int i;
    int a[LENGTH], b[LENGTH], c[LENGTH];
    int golden[LENGTH];
    int length = LENGTH;
    int result = EXIT_SUCCESS;

    for(i = 0; i < length; i++){
      a[i] = i;
      b[i] = i + 5;
    }

    for(i = 0; i < length; i++) {
        golden[i] = a[i] + b[i];
    }

    vadd_tl(a, b, c, length); 

    for(i = 0; i < length; i++) {
        if(golden[i] != c[i]) {
            printf("vadd_tl doesn't match golden output at index %d\n", i);
            result = EXIT_FAILURE;
            break;
        }
    }
    if(i == length) {
        printf("Success!\n");
    }

    printf("A = [");
    print_vec(a, length);
    printf("]\n");
        
    printf("B = [");
    print_vec(b, length);
    printf("]\n");
 
    printf("C = [");
    print_vec(c, length);
    printf("]\n");
 
    return result;
}
