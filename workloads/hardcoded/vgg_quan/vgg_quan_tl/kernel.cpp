#include "ap_int.h"
#include "hls_stream.h"
#include "conv2d.h"
#include "dma.h"
#include "param.h"
#include "vgg_layer.h"
      
	extern "C" 
 void vgg0(const ap_uint<IFMAP_DW> fmap[REP * DIM_LAYER0*DIM_LAYER0*CH_LAYER0], 
		const ap_uint<WEIGHT_DW*PE> k1[F][F][CH_LAYER0][CH_LAYER1/PE],
		const ap_uint<WEIGHT_DW*PE> k2[F][F][CH_LAYER1][CH_LAYER1/PE],
		 hls::stream<ap_uint<FMAP_DW> > &max_pool1,
 const int rep){

#pragma HLS INTERFACE s_axilite port=rep bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control
#pragma HLS DATAFLOW
        
#pragma HLS INTERFACE m_axi port=fmap bundle=gmem
#pragma HLS INTERFACE s_axilite port=fmap bundle=control
    
#pragma HLS INTERFACE m_axi port=out bundle=gmem0
#pragma HLS INTERFACE s_axilite port=out bundle=control
    
#pragma HLS INTERFACE m_axi port=k1 bundle=gmem1
#pragma HLS INTERFACE s_axilite port=k1 bundle=control
#pragma HLS INTERFACE m_axi port=k2 bundle=gmem2
#pragma HLS INTERFACE s_axilite port=k2 bundle=control
    


	hls::stream<ap_uint<IFMAP_DW> > st_layer0;
	M2S<DIM_LAYER0*DIM_LAYER0*CH_LAYER0, ap_uint<IFMAP_DW>, ap_uint<IFMAP_DW> >(fmap, st_layer0, rep);
		const T_SUM th1[(1 << FMAP_DW)-1]={
#include "th.txt"
		};
		const T_SUM th2[(1 << FMAP_DW)-1]={
#include "th.txt"
		};
	hls::stream<ap_uint<FMAP_DW> > st_layer1;
	vgg_layer<DIM_LAYER1, CH_LAYER0, CH_LAYER1>(st_layer0, k1, th1, st_layer1, rep);

	hls::stream<ap_uint<FMAP_DW> > st_layer2;
	vgg_layer<DIM_LAYER1, CH_LAYER1, CH_LAYER1>(st_layer1, k2, th2, st_layer2, rep);

	max_pool<DIM_LAYER1,CH_LAYER1,2>(st_layer2, max_pool1, rep);
		
}