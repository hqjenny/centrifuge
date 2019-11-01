#include<stdio.h>
#include<stdint.h>
#define LENGTH 80
#ifdef CUSTOM_DRIVER
#include "bm_wrapper.h"
#endif
#include "time.h"

void print_vec(int* vec, int length){
    for(int i = 0; i < length; i++){
        if (i != 0 ) printf(", ");
        printf("%d", vec[i]);
    }
}

//Including to use ap_uint<> datatype
//#include <ap_int.h>
/*
#define BUFFER_SIZE 128
#define DATAWIDTH   512
#define VECTOR_SIZE (DATAWIDTH / 32) // vector size is 16 (512/32 = 16)
//typedef ap_uint<DATAWIDTH> uint512_dt;

    Vector Addition Kernel Implementation using uint512_dt datatype 
    Arguments:
        in1   (input)     --> Input Vector1
        in2   (input)     --> Input Vector2
        out   (output)    --> Output Vector
        size  (input)     --> Size of Vector in Integer
   */
/*extern "C" {
void vadd(
        const uint512_dt *in1, // Read-Only Vector 1
        const uint512_dt *in2, // Read-Only Vector 2
        uint512_dt *out,       // Output Result
        int size               // Size in integer
        )
{
#pragma HLS INTERFACE m_axi port=in1  offset=slave bundle=gmem
#pragma HLS INTERFACE m_axi port=in2  offset=slave bundle=gmem
#pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem
#pragma HLS INTERFACE s_axilite port=in1  bundle=control
#pragma HLS INTERFACE s_axilite port=in2  bundle=control
#pragma HLS INTERFACE s_axilite port=out bundle=control
#pragma HLS INTERFACE s_axilite port=size bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    uint512_dt v1_local[BUFFER_SIZE];    // Local memory to store vector1
    uint512_dt result_local[BUFFER_SIZE];// Local Memory to store result

    // Input vector size for interger vectors. However kernel is directly 
    // accessing 512bit data (total 16 elements). So total number of read
    // from global memory is calculated here:
    int size_in16 = (size-1) / VECTOR_SIZE + 1; 

    //Per iteration of this loop perform BUFFER_SIZE vector addition
    for(int i = 0; i < size_in16;  i += BUFFER_SIZE)
    {
        #pragma HLS LOOP_TRIPCOUNT min=8 max=8
        int chunk_size = BUFFER_SIZE;

        //boundary checks
        if ((i + BUFFER_SIZE) > size_in16) 
            chunk_size = size_in16 - i;

        //burst read first vector from global memory to local memory
        v1_rd: for (int j = 0 ; j <  chunk_size; j++){
        #pragma HLS pipeline
        #pragma HLS LOOP_TRIPCOUNT min=128 max=128
            v1_local[j] = in1 [i + j];
        }

        //burst read second vector and perform vector addition
        v2_rd_add: for (int j = 0 ; j < chunk_size; j++){
        #pragma HLS pipeline
        #pragma HLS LOOP_TRIPCOUNT min=128 max=128
            uint512_dt tmpV1     = v1_local[j];
            uint512_dt tmpV2     = in2[i+j];
            result_local[j] = tmpV1 + tmpV2; // Vector Addition Operation
        }

        //burst write the result
        out_write: for (int j = 0 ; j < chunk_size; j++){
        #pragma HLS pipeline
        #pragma HLS LOOP_TRIPCOUNT min=128 max=128
            out[i+j] = result_local[j];
       }
    }
}
}*/

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
        // To prevent burst mode
        c[i+0] = a[i+0] +b[i+0];
        c[i+1] = a[i+1] +b[i+1];
        c[i+2] = a[i+2] +b[i+2];
        c[i+3] = a[i+3] +b[i+3];

        c[i+4] = a[i+4] +b[i+4];
        c[i+5] = a[i+5] +b[i+5];
        c[i+6] = a[i+6] +b[i+6];
        c[i+7] = a[i+7] +b[i+7];
    }
    
    int output = 0;
    for (i = upper; i < length; i++) {
        c[i] = a[i] +b[i];
    }
    return 0;
}

int main () {

    int a[LENGTH], b[LENGTH], c[LENGTH];
    int length = LENGTH;
    int i;
    for(i = 0; i < length; i++){
      a[i] = i;
      b[i] = i + 5;
    }
uint64_t begin, end, dur;

begin = read_cycle();
#ifdef CUSTOM_DRIVER
    vadd_wrapper(a, b, c, length); 
#else
    vadd(a, b, c, length); 
#endif 
end = read_cycle();
duration(begin, end);
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
