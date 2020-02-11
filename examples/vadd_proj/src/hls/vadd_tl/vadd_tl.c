#include<stdio.h>
#include<stdint.h>
#define LENGTH 80
#include "accel.h"

void print_vec(int* vec, int length){
    for(int i = 0; i < length; i++){
        if (i != 0 ) printf(", ");
        printf("%d", vec[i]);
    }
}

int main () {

    int a[LENGTH], b[LENGTH], c[LENGTH];
    int length = LENGTH;
    int i;
    for(i = 0; i < length; i++){
      a[i] = i;
      b[i] = i + 5;
    }
    uint64_t begin, end;

    vadd(a, b, c, length); 
    printf("A = [");
    print_vec(a, length);
    printf("]\n");
        
    printf("B = [");
    print_vec(b, length);
    printf("]\n");
 
    printf("C = [");
    print_vec(c, length);
    printf("]\n");
 
    return 0;
}
