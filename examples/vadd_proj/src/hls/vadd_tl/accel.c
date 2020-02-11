#include "accel.h"

int vadd(int * a, int * b, int* c, int length) {

// For pointer type
#pragma HLS INTERFACE m_axi port=a offset=slave bundle=gmem0 num_write_outstanding=16 num_read_outstanding=16 max_write_burst_length=16 max_read_burst_length=    16 depth=16 latency=125

#pragma HLS INTERFACE m_axi port=b offset=slave bundle=gmem0 num_write_outstanding=16 num_read_outstanding=16 max_write_burst_length=16 max_read_burst_length=    16 depth=16 latency=125
 // Slave is for AXI4Lite, with burst mode disabled
#pragma HLS INTERFACE m_axi port=c offset=slave bundle=gmem0  num_write_outstanding=16 num_read_outstanding=16 max_write_burst_length=16 max_read_burst_length=    16 depth=16 latency=125


#pragma HLS INTERFACE s_axilite port=a bundle=control
#pragma HLS INTERFACE s_axilite port=b bundle=control
#pragma HLS INTERFACE s_axilite port=c bundle=control
#pragma HLS INTERFACE s_axilite port=length bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

//#pragma HLS DATAFLOW
    int upper = (length >> 3) << 3;
    int i = 0;
    for (i = 0; i < upper; i += 8) {
        c[i+0] = a[i+0] +b[i+0];
        c[i+1] = a[i+1] +b[i+1];
        c[i+2] = a[i+2] +b[i+2];
        c[i+3] = a[i+3] +b[i+3];

        c[i+4] = a[i+4] +b[i+4];
        c[i+5] = a[i+5] +b[i+5];
        c[i+6] = a[i+6] +b[i+6];
        c[i+7] = a[i+7] +b[i+7];
    }
    
    for (i = upper; i < length; i++) {
        c[i] = a[i] +b[i];
    }
    return 0;
}

