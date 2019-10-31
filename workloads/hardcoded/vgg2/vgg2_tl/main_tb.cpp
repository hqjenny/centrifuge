#include <iostream>
using namespace std;
#include "ap_int.h"
#include "hls_stream.h"
#include "param.h"
#include "tb.h"
using namespace TB;
extern "C" 
 void vgg0(const ap_uint<IFMAP_DW> fmap[REP * DIM_LAYER0*DIM_LAYER0*CH_LAYER0],  hls::stream<ap_uint<FMAP_DW> >& max_pool3,
 const int rep);
extern "C" 
 void vgg1(hls::stream<ap_uint<FMAP_DW> > &max_pool3,  ap_uint<OUT_DW> out[REP * FC_LAYER2*FMAP_DW/OUT_DW],
 const int rep);
  int main(){
  
hls::stream<ap_uint<FMAP_DW> > max_pool2;
vgg0(fmap, k1,k2,k3,k4,k5,k6,k7, max_pool2, rep);
vgg1(max_pool2, k8,k9,k10,k11,k12,k13,k14,k15,k16, out, rep);
return 0;
}