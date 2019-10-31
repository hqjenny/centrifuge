//#ifdef CUSTOM_DRIVER
#include "bm_wrapper.h"
//#endif
#include "time.h"

#define IF_DIM 128
#define IF_CH 64
#define IN_W 8
#define OUT_W 8

#define FM_W 4
#define N_TH (1<<FM_W)
#define W_W 1
#define SUM_W 13

//#define PA_0 32
//#define PE_0 16

#define PA_0 8
#define PE_0 8

#define TEST_D 16
#define TEST_C 512
#define REP 1

static char fmap[REP*TEST_D*TEST_D*TEST_C/PA_0 *FM_W*PA_0/8];
static char out[REP*TEST_D*TEST_D*TEST_C/PE_0 * FM_W*PA_0/8];
static char k0[TEST_C*TEST_C/PE_0/PA_0 * W_W * PE_0 * PA_0 / 8];
static char k1[TEST_C*TEST_C/PE_0/PA_0 * W_W * PE_0 * PA_0 / 8];
static char k2[TEST_C*TEST_C/PE_0/PA_0 * W_W * PE_0 * PA_0 / 8];

static char fmap_0[REP*64*64*128*FM_W/8];
static char out_0[REP*64*64*128*FM_W/8];
static char k0_0[128*128*W_W/8];
static char k1_0[128*128*W_W/8];
static char k2_0[128*128*W_W/8];
static char fmap_1[REP*32*32*128*FM_W/8];
static char out_1[REP*32*32*128*FM_W/8];
static char k0_1[128*128*W_W/8];
static char k1_1[128*128*W_W/8];
static char k2_1[128*128*W_W/8];
static char fmap_2[REP*32*32*128*FM_W/8];
static char out_2[REP*32*32*128*FM_W/8];
static char k0_2[128*128*W_W/8];
static char k1_2[128*128*W_W/8];
static char k2_2[128*128*W_W/8];
static char fmap_3[REP*32*32*128*FM_W/8];
static char out_3[REP*32*32*128*FM_W/8];
static char k0_3[128*128*W_W/8];
static char k1_3[128*128*W_W/8];
static char k2_3[128*128*W_W/8];
static char fmap_4[REP*32*32*256*FM_W/8];
static char out_4[REP*32*32*256*FM_W/8];
static char k0_4[256*256*W_W/8];
static char k1_4[256*256*W_W/8];
static char k2_4[256*256*W_W/8];
static char fmap_5[REP*16*16*256*FM_W/8];
static char out_5[REP*16*16*256*FM_W/8];
static char k0_5[256*256*W_W/8];
static char k1_5[256*256*W_W/8];
static char k2_5[256*256*W_W/8];
static char fmap_6[REP*16*16*256*FM_W/8];
static char out_6[REP*16*16*256*FM_W/8];
static char k0_6[256*256*W_W/8];
static char k1_6[256*256*W_W/8];
static char k2_6[256*256*W_W/8];
static char fmap_7[REP*16*16*256*FM_W/8];
static char out_7[REP*16*16*256*FM_W/8];
static char k0_7[256*256*W_W/8];
static char k1_7[256*256*W_W/8];
static char k2_7[256*256*W_W/8];
static char fmap_8[REP*16*16*256*FM_W/8];
static char out_8[REP*16*16*256*FM_W/8];
static char k0_8[256*256*W_W/8];
static char k1_8[256*256*W_W/8];
static char k2_8[256*256*W_W/8];
static char fmap_9[REP*16*16*256*FM_W/8];
static char out_9[REP*16*16*256*FM_W/8];
static char k0_9[256*256*W_W/8];
static char k1_9[256*256*W_W/8];
static char k2_9[256*256*W_W/8];
static char fmap_10[REP*16*16*256*FM_W/8];
static char out_10[REP*16*16*256*FM_W/8];
static char k0_10[256*256*W_W/8];
static char k1_10[256*256*W_W/8];
static char k2_10[256*256*W_W/8];
static char fmap_11[REP*16*16*256*FM_W/8];
static char out_11[REP*16*16*256*FM_W/8];
static char k0_11[256*256*W_W/8];
static char k1_11[256*256*W_W/8];
static char k2_11[256*256*W_W/8];
static char fmap_12[REP*16*16*512*FM_W/8];
static char out_12[REP*16*16*512*FM_W/8];
static char k0_12[512*512*W_W/8];
static char k1_12[512*512*W_W/8];
static char k2_12[512*512*W_W/8];
static char fmap_13[REP*8*8*512*FM_W/8];
static char out_13[REP*8*8*512*FM_W/8];
static char k0_13[512*512*W_W/8];
static char k1_13[512*512*W_W/8];
static char k2_13[512*512*W_W/8];
static char fmap_14[REP*8*8*512*FM_W/8];
static char out_14[REP*8*8*512*FM_W/8];
static char k0_14[512*512*W_W/8];
static char k1_14[512*512*W_W/8];
static char k2_14[512*512*W_W/8];
static char fmap_15[REP*8*8*512*FM_W/8];
static char out_15[REP*8*8*512*FM_W/8];
static char k0_15[512*512*W_W/8];
static char k1_15[512*512*W_W/8];
static char k2_15[512*512*W_W/8];


main (){
  //int rep = 1;
  uint64_t begin, end, dur;
//	for(int i=0;i<TEST_D*TEST_D*TEST_C/PA_0;i++)
//		fmap[i] = rand();
//	for(int i=0;i<TEST_C*TEST_C/PA_0/PE_0;i++)
//		k0[i] = rand();
//	for(int i=0;i<TEST_C*TEST_C/PA_0/PE_0;i++)
//		k1[i] = rand();
  begin = read_cycle();
//  #ifdef CUSTOM_DRIVER
  // 
//  top_wrapper( fmap,  out,  k0,  k1,  k2, TEST_D, TEST_C, 0, 0, REP);
top_wrapper( fmap_0, out_0, k0_0, k1_0, k2_0, 64, 128, 0, 1, REP);
top_wrapper( fmap_1, out_1, k0_1, k1_1, k2_1, 32, 128, 0, 0, REP);
top_wrapper( fmap_2, out_2, k0_2, k1_2, k2_2, 32, 128, 0, 0, REP);
top_wrapper( fmap_3, out_3, k0_3, k1_3, k2_3, 32, 128, 0, 0, REP);
top_wrapper( fmap_4, out_4, k0_4, k1_4, k2_4, 32, 256, 0, 1, REP);
top_wrapper( fmap_5, out_5, k0_5, k1_5, k2_5, 16, 256, 0, 0, REP);
top_wrapper( fmap_6, out_6, k0_6, k1_6, k2_6, 16, 256, 0, 0, REP);
top_wrapper( fmap_7, out_7, k0_7, k1_7, k2_7, 16, 256, 0, 0, REP);
top_wrapper( fmap_8, out_8, k0_8, k1_8, k2_8, 16, 256, 0, 0, REP);
top_wrapper( fmap_9, out_9, k0_9, k1_9, k2_9, 16, 256, 0, 0, REP);
top_wrapper( fmap_10, out_10, k0_10, k1_10, k2_10, 16, 256, 0, 0, REP);
top_wrapper( fmap_11, out_11, k0_11, k1_11, k2_11, 16, 256, 0, 0, REP);
top_wrapper( fmap_12, out_12, k0_12, k1_12, k2_12, 16, 512, 0, 1, REP);
top_wrapper( fmap_13, out_13, k0_13, k1_13, k2_13, 8, 512, 0, 0, REP);
top_wrapper( fmap_14, out_14, k0_14, k1_14, k2_14, 8, 512, 0, 0, REP);
top_wrapper( fmap_15, out_15, k0_15, k1_15, k2_15, 8, 512, 0, 0, REP);
//  top_wrapper( fmap,  out,  k0,  k1,  k2, TEST_D, TEST_C, 0, 0, REP);
//  #else

//  #endif 
  end = read_cycle();
  duration(begin, end);
}
