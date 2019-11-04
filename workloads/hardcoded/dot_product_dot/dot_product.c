#include<stdio.h>
#include<stdint.h>
#define LENGTH 80
#define NUMBER_OF_INPUT 784
#include "time.h"
#ifdef CUSTOM_INST
#include "bm_wrapper.h"
#endif


double dot(double a[NUMBER_OF_INPUT], double b[NUMBER_OF_INPUT]) {
#pragma HLS INTERFACE ap_bus depth=10 port=a
#pragma HLS INTERFACE ap_bus depth=10 port=b
    double result[NUMBER_OF_INPUT];
    int i;
    for (i = 0; i < (NUMBER_OF_INPUT >> 3) << 3 ; i += 8) {
    #pragma HLS PIPELINE

        // To prevent burst mode
        result[0] += a[i] *b[i];
        result[1] += a[i+1] *b[i+1];
        result[2] += a[i+2] *b[i+2];
        result[3] += a[i+3] *b[i+3];

        result[4] += a[i+4] *b[i+4];
        result[5] += a[i+5] *b[i+5];
        result[6] += a[i+6] *b[i+6];
        result[7] += a[i+7] *b[i+7];


    }
    double output = 0;
    for (i = (NUMBER_OF_INPUT >> 3) << 3; i < NUMBER_OF_INPUT; i += 1) {
        output += a[i] * b[i];
    }

    result[0] = result[0] + result[1];
    result[2] = result[2] + result[3];
    result[4] = result[4] + result[5];
    result[6] = result[6] + result[7];

    result[0] = result[0] + result[2];
    result[4] = result[4] + result[6];

    output = result[0] + result[4];

    printf("output=%d\n", (int) output);
    return output;
}



int main () {

    double a[NUMBER_OF_INPUT], b[NUMBER_OF_INPUT];
    int i;
    for(i = 0; i < NUMBER_OF_INPUT; i++){
      a[i] = (double)i;
      b[i] = (double)i + 5.0;
    }

    double c; 

    uint64_t start = read_cycle();
#ifdef CUSTOM_INST
    c = dot_wrapper(a, b);
#else
    c = dot(a, b);
#endif 
    duration(start, read_cycle());

    printf("A . B = %x\n", c);
 
    return 0;
}
