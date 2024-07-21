#include<stdio.h>
#include<stdint.h>
#ifdef CUSTOM_DRIVER
#include "bm_wrapper.h"
#endif
#include "time.h"

#define LENGTH 80

int dot(int* a, int* b, int length) {
// For pointer type
#pragma HLS INTERFACE m_axi port=a offset=slave bundle=gmem0 // Direct is for AXI with full 32 bit address space
#pragma HLS INTERFACE m_axi port=b offset=slave bundle=gmem0 // Slave is for AXI4Lite, with burst mode disabled

#pragma HLS INTERFACE s_axilite port=a bundle=control
#pragma HLS INTERFACE s_axilite port=b bundle=control
#pragma HLS INTERFACE s_axilite port=length bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

//#pragma HLS DATAFLOW
    // result buffer
    int result[8] = {0,0,0,0,0,0,0,0};
    #pragma HLS ARRAY_PARTITION variable=result complete

    // Unroll bound 
    int upper = (length >> 3) << 3;
    int i = 0;
    for (i = 0; i < upper; i += 8) {
    //#pragma HLS PIPELINE
        // To prevent burst mode
        result[0] += a[i] *b[i];
        result[1] += a[i+1] *b[i+1];
        result[2] += a[i+2] *b[i+2];
        result[3] += a[i+3] *b[i+3];

        result[4] += a[i+4] *b[i+4];
        result[5] += a[i+5] *b[i+5];
        result[6] += a[i+6] *b[i+6];
        result[7] += a[i+7] *b[i+7];

        //printf("input_a: %d %d %d %d %d %d %d %d\n", a[i], a[i+1], a[i+2], a[i+3], a[i+4], a[i+5], a[i+6], a[i+7]);;
        //printf("input_b: %d %d %d %d %d %d %d %d\n", b[i], b[i+1], b[i+2], b[i+3], b[i+4], b[i+5], b[i+6], b[i+7]);;
    }

    
    int output = 0;

    for (i = upper; i < length; i++) {
        output +=  a[i] * b[i];
    }

    result[0] = result[0] + result[1];
    result[2] = result[2] + result[3];
    result[4] = result[4] + result[5];
    result[6] = result[6] + result[7];

    result[0] = result[0] + result[2];
    result[4] = result[4] + result[6];

    output = result[0] + result[4];

    return output;
}

int main () {

    int a[LENGTH], b[LENGTH];
    int length = LENGTH;
    int i;
    for(i = 0; i < LENGTH; i++){
      a[i] = i;
      b[i] = i + 5;
    }

    int c; 

uint64_t begin, end, dur;
begin = read_cycle();
#ifdef CUSTOM_DRIVER
    c = dot_wrapper(a, b, length); 
#else
    c = dot(a, b, length); 
#endif 
end = read_cycle();
duration(begin, end);

    printf("A . B = %d\n", c);
 
    return 0;
}
