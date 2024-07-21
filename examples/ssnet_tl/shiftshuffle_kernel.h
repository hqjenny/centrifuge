#include<iostream>

#include "/ecad/tools/xilinx/Vivado/2017.4/include/gmp.h"
//#include "D:\software\vivado\Vivado\2018.2\include\gmp.h"

#include "hls_stream.h"
#include "ap_int.h"
using namespace std;

#define OC 12
#define IC 12 
#define N 8
#define IC_VEC 6

#define TOTAL_OC 24
#define TOTAL_IC 12
#define HEIGHT 32
#define WIDTH 32
#define ACT_BIT 4

#define OUT_ACT_NUM 15

// oc_size is (TOTAL_OC / OC)
// ic_size is (TOTAL_IC / IC)
// width is WIDTH
// height is HEIGHT

void top(ap_uint<256>* threshold, ap_uint<128>* in_fmap, ap_uint<512>* weight, //multiplicate of 8
		ap_uint<128>* out_fmap,
	    unsigned int in_fmap_w,
	    unsigned int in_fmap_h,
	    unsigned int ic_size,
	    unsigned int oc_size,
		unsigned int is_shift,
		unsigned int is_pool
);

ap_uint<4> convert(ap_uint<13> input, ap_uint<13> threshold[OUT_ACT_NUM]);
