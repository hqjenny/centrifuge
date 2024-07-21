#include "accel.h"

int vadd(int* length_a, int* b_c) {
#pragma HLS INTERFACE ap_bus depth=10 port=length_a
#pragma HLS INTERFACE ap_bus depth=10 port=b_c
 
int length = length_a[0];
int * a = & length_a[1];
//int * b = b_c;
//int * c = & b_c[length];
// For pointer type

//#pragma HLS DATAFLOW
    int upper = (length >> 3) << 3;
    int i = 0;
    for (i = 0; i < upper; i += 8) {
        // To prevent burst mode
        b_c[length+i+1] = a[i+1] +b_c[i+1];
        b_c[length+i+0] = a[i+0] +b_c[i+0];
        b_c[length+i+2] = a[i+2] +b_c[i+2];
        b_c[length+i+3] = a[i+3] +b_c[i+3];

        b_c[length+i+4] = a[i+4] +b_c[i+4];
        b_c[length+i+5] = a[i+5] +b_c[i+5];
        b_c[length+i+6] = a[i+6] +b_c[i+6];
        b_c[length+i+7] = a[i+7] +b_c[i+7];
    }
    
    int output = 0;
    for (i = upper; i < length; i++) {
        b_c[length+i] = a[i] +b_c[i];
    }
    return 0;
}


