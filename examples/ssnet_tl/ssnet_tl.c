//#ifdef CUSTOM_DRIVER
#include "bm_wrapper.h"
//#endif
#include "time.h"
#define OC 12
#define IC 12 
#define N 8
#define IC_VEC 6

#define TOTAL_OC 64
#define TOTAL_IC 64
#define HEIGHT 8
#define WIDTH 8
#define ACT_BIT 4

#define OUT_ACT_NUM 15

    int threshold[15];
    int weight[TOTAL_OC*TOTAL_IC/OC/IC * 512];
    int in_fmap[TOTAL_IC*HEIGHT * 128];
    //ap_uint<36> in_fmap_bram[8][9];
    //the first is how many N, second is different bit for 13 bits, third is oc
    //ap_uint<36> partial_sum_bram0[4][4][OC];
    int out_fmap[TOTAL_OC*HEIGHT * 128];



main (){

    int is_shift = 0;
    int is_pool = 0;

    int in_fmap_w = WIDTH;
    int in_fmap_h = HEIGHT;

    int ic_size = TOTAL_IC;
    int oc_size = TOTAL_OC;

    uint64_t begin, end, dur;
    begin = read_cycle();
    top_wrapper( threshold,  in_fmap,  weight,  out_fmap,  in_fmap_w,  in_fmap_h,  ic_size,  oc_size,  is_shift,  is_pool);
//  #ifdef CUSTOM_DRIVER
//  #else
//  #endif 
    end = read_cycle();
    duration(begin, end);
}
