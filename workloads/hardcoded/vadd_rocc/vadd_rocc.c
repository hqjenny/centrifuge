#include<stdio.h>
#include "../os_utils.h"
#define LENGTH 8

#ifdef CUSTOM_INST 
#include "rocc.h"
#endif

#include "../custom_mmap/mmap_driver.c"
void print_vec(int* vec, int length){
    for(int i = 0; i < length; i++){
        if (i != 0 ) printf(", ");
        printf("%d", vec[i]);
    }
}

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

int main () {

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
begin = read_cycle();
volatile int block = 0;
#ifdef CUSTOM_INST
    #define XCUSTOM_ACC 0
	asm volatile ("fence");

	//ROCC_INSTRUCTION_DSS(XCUSTOM_ACC, block, vtop_translate(length_a), vtop_translate(b_c), 0);
	//ROCC_INSTRUCTION_DSS(XCUSTOM_ACC, block, vtop_translate(length_a), vtop_translate(b_c), 0);
	ROCC_INSTRUCTION_DSS(XCUSTOM_ACC, block, length_a, b_c, 0);

	asm volatile ("fence");
#else
    vadd(length_a, b_c); 
#endif 
//length_a[0] = block;
end = read_cycle();
duration(begin, end);

//    printf("A = [");
//    print_vec(&length_a[1], length);
//    printf("]\n");
//        
//    printf("B = [");
//    print_vec(b_c, length);
//    printf("]\n");
// 
//    printf("C = [");
//    print_vec(&b_c[length], length);
//    printf("]\n");
 
    return 0;
}
