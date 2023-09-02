//#pragma once
#include "ap_int.h"

#include "/ecad/tools/xilinx/Vivado/2018.2/include/gmp.h"
//#include "ap_int.h"
#include "hls_stream.h"

using namespace std;
#define IF_DIM 128
#define IF_CH 64
#define IN_W 8
#define OUT_W 8
typedef ap_uint<IN_W> T_IN;
typedef ap_uint<OUT_W> T_OUT;

#define FM_W 4
typedef ap_uint<FM_W> T_FMAP;
#define N_TH (1<<FM_W)
#define W_W 1
typedef ap_uint<W_W> T_W;
#define SUM_W 13
typedef ap_uint<SUM_W> T_SUM;

//#define PA_0 32
//#define PE_0 16

#define PA_0 8
#define PE_0 8

#define TEST_D 32
#define TEST_C 128
#define REP 4
