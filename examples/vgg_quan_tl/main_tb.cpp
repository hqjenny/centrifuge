#include <iostream>
using namespace std;
#include "ap_int.h"
#include "hls_stream.h"
#include "param.h"
#include "tb.h"
using namespace TB;
extern "C" 
 void vgg0(const ap_uint<IFMAP_DW> fmap[REP * DIM_LAYER0*DIM_LAYER0*CH_LAYER0], 
		const ap_uint<WEIGHT_DW*PE> k1[F][F][CH_LAYER0][CH_LAYER1/PE],
		const ap_uint<WEIGHT_DW*PE> k2[F][F][CH_LAYER1][CH_LAYER1/PE],
		 hls::stream<ap_uint<FMAP_DW> > &max_pool1,
 const int rep);
  int main(){
  
vgg0(fmap, k1,k2, max_pool0, rep);
return 0;
}