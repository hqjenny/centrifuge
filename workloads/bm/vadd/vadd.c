#include<stdio.h>
#include<stdint.h>
#include "time.h"
#define LENGTH 80

#include "accel.h"

void print_vec(int* vec, int length){
    for(int i = 0; i < length; i++){
        if (i != 0 ) printf(", ");
        printf("%d", vec[i]);
    }
}

int main () {
    int length_a[LENGTH + 1] __attribute__((aligned(64)));
    int b_c[LENGTH + LENGTH] __attribute__((aligned(64)));
    //int a[LENGTH], b[LENGTH], c[LENGTH];
    int length = LENGTH;
    length_a[0] = length;
    int i;
    for(i = 0; i < length; i++){
      length_a[i + 1] = i;
      b_c[i] = i + 5;
    }

    printf("length_a ptr: %X", length_a);
    uint64_t begin, end, dur;
    begin = read_cycle();
    vadd(length_a, b_c); 
    end = read_cycle();
    duration(begin, end);

    printf("A = [");
    print_vec(&length_a[1], length);
    printf("]\n");
        
    printf("B = [");
    print_vec(b_c, length);
    printf("]\n");
 
    printf("C = [");
    print_vec(&b_c[length], length);
    printf("]\n");

    return 0;
}
