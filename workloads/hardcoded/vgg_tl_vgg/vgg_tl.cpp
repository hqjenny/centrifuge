#include "ap_int.h"
#include "hls_stream.h"
#include "conv2d.h"
#include "dma.h"
#include "param.h"
#include "vgg_layer.h"
      
	extern "C" 
 void vgg(const ap_uint<IFMAP_DW> fmap[REP * DIM_LAYER0*DIM_LAYER0*CH_LAYER0], 
		 ap_uint<OUT_DW> out[REP * FC_LAYER2*FMAP_DW/OUT_DW],
 const int rep){

#pragma HLS INTERFACE s_axilite port=rep bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control
#pragma HLS DATAFLOW
        
#pragma HLS INTERFACE m_axi port=fmap bundle=gmem
#pragma HLS INTERFACE s_axilite port=fmap bundle=control
    
#pragma HLS INTERFACE m_axi port=out bundle=gmem0
#pragma HLS INTERFACE s_axilite port=out bundle=control
    
//#pragma HLS INTERFACE m_axi port=k1 bundle=gmem1
//#pragma HLS INTERFACE s_axilite port=k1 bundle=control
//#pragma HLS INTERFACE m_axi port=k2 bundle=gmem2
//#pragma HLS INTERFACE s_axilite port=k2 bundle=control
//    
//#pragma HLS INTERFACE m_axi port=k3 bundle=gmem3
//#pragma HLS INTERFACE s_axilite port=k3 bundle=control
//#pragma HLS INTERFACE m_axi port=k4 bundle=gmem4
//#pragma HLS INTERFACE s_axilite port=k4 bundle=control
//    
//#pragma HLS INTERFACE m_axi port=k5 bundle=gmem5
//#pragma HLS INTERFACE s_axilite port=k5 bundle=control
//#pragma HLS INTERFACE m_axi port=k6 bundle=gmem6
//#pragma HLS INTERFACE s_axilite port=k6 bundle=control
//#pragma HLS INTERFACE m_axi port=k7 bundle=gmem7
//#pragma HLS INTERFACE s_axilite port=k7 bundle=control
//    
//#pragma HLS INTERFACE m_axi port=k8 bundle=gmem8
//#pragma HLS INTERFACE s_axilite port=k8 bundle=control
//#pragma HLS INTERFACE m_axi port=k9 bundle=gmem9
//#pragma HLS INTERFACE s_axilite port=k9 bundle=control
//#pragma HLS INTERFACE m_axi port=k10 bundle=gmem10
//#pragma HLS INTERFACE s_axilite port=k10 bundle=control
//    
//#pragma HLS INTERFACE m_axi port=k11 bundle=gmem11
//#pragma HLS INTERFACE s_axilite port=k11 bundle=control
//#pragma HLS INTERFACE m_axi port=k12 bundle=gmem12
//#pragma HLS INTERFACE s_axilite port=k12 bundle=control
//#pragma HLS INTERFACE m_axi port=k13 bundle=gmem13
//#pragma HLS INTERFACE s_axilite port=k13 bundle=control
//    
//#pragma HLS INTERFACE m_axi port=k14 bundle=gmem14
//#pragma HLS INTERFACE s_axilite port=k14 bundle=control
//#pragma HLS INTERFACE m_axi port=k15 bundle=gmem15
//#pragma HLS INTERFACE s_axilite port=k15 bundle=control
//#pragma HLS INTERFACE m_axi port=k16 bundle=gmem16
//#pragma HLS INTERFACE s_axilite port=k16 bundle=control
    
		const ap_uint<WEIGHT_DW*PE> k1[F][F][CH_LAYER0][CH_LAYER1/PE];
		const ap_uint<WEIGHT_DW*PE> k2[F][F][CH_LAYER1][CH_LAYER1/PE];
		
		const ap_uint<WEIGHT_DW*PE> k3[F][F][CH_LAYER1][CH_LAYER2/PE];
		const ap_uint<WEIGHT_DW*PE> k4[F][F][CH_LAYER2][CH_LAYER2/PE];
		
		const ap_uint<WEIGHT_DW*PE> k5[F][F][CH_LAYER2][CH_LAYER3/PE];
		const ap_uint<WEIGHT_DW*PE> k6[F][F][CH_LAYER3][CH_LAYER3/PE];
		const ap_uint<WEIGHT_DW*PE> k7[F][F][CH_LAYER3][CH_LAYER3/PE];
		
		const ap_uint<WEIGHT_DW*PE> k8[F][F][CH_LAYER3][CH_LAYER4/PE];
		const ap_uint<WEIGHT_DW*PE> k9[F][F][CH_LAYER4][CH_LAYER4/PE];
		const ap_uint<WEIGHT_DW*PE> k10[F][F][CH_LAYER4][CH_LAYER4/PE];
		
		const ap_uint<WEIGHT_DW*PE> k11[F][F][CH_LAYER4][CH_LAYER5/PE];
		const ap_uint<WEIGHT_DW*PE> k12[F][F][CH_LAYER5][CH_LAYER5/PE];
		const ap_uint<WEIGHT_DW*PE> k13[F][F][CH_LAYER5][CH_LAYER5/PE];
		
		const ap_uint<WEIGHT_DW*PE> k14[FC_DIM0 * FC_DIM0 *CH_LAYER5][FC_LAYER1/PE];
		const ap_uint<WEIGHT_DW*PE> k15[FC_LAYER1][FC_LAYER1/PE];
		const ap_uint<WEIGHT_DW*PE> k16[FC_LAYER1][FC_LAYER2/PE];


hls::stream<ap_uint<FMAP_DW> > max_pool1;

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
		

hls::stream<ap_uint<FMAP_DW> > max_pool2;

		const T_SUM th3[(1 << FMAP_DW)-1]={
#include "th.txt"
		};
		const T_SUM th4[(1 << FMAP_DW)-1]={
#include "th.txt"
		};
	hls::stream<ap_uint<FMAP_DW> > st_layer3;
	vgg_layer<DIM_LAYER2, CH_LAYER1, CH_LAYER2>(max_pool1, k3, th3, st_layer3, rep);

	hls::stream<ap_uint<FMAP_DW> > st_layer4;
	vgg_layer<DIM_LAYER2, CH_LAYER2, CH_LAYER2>(st_layer3, k4, th4, st_layer4, rep);

	max_pool<DIM_LAYER2,CH_LAYER2,2>(st_layer4, max_pool2, rep);
		

hls::stream<ap_uint<FMAP_DW> > max_pool3;

		const T_SUM th5[(1 << FMAP_DW)-1]={
#include "th.txt"
		};
		const T_SUM th6[(1 << FMAP_DW)-1]={
#include "th.txt"
		};
		const T_SUM th7[(1 << FMAP_DW)-1]={
#include "th.txt"
		};
	hls::stream<ap_uint<FMAP_DW> > st_layer5;
	vgg_layer<DIM_LAYER3, CH_LAYER2, CH_LAYER3>(max_pool2, k5, th5, st_layer5, rep);

	hls::stream<ap_uint<FMAP_DW> > st_layer6;
	vgg_layer<DIM_LAYER3, CH_LAYER3, CH_LAYER3>(st_layer5, k6, th6, st_layer6, rep);

	hls::stream<ap_uint<FMAP_DW> > st_layer7;
	vgg_layer<DIM_LAYER3, CH_LAYER3, CH_LAYER3>(st_layer6, k7, th7, st_layer7, rep);

	max_pool<DIM_LAYER3,CH_LAYER3,2>(st_layer7, max_pool3, rep);
		

hls::stream<ap_uint<FMAP_DW> > max_pool4;

		const T_SUM th8[(1 << FMAP_DW)-1]={
#include "th.txt"
		};
		const T_SUM th9[(1 << FMAP_DW)-1]={
#include "th.txt"
		};
		const T_SUM th10[(1 << FMAP_DW)-1]={
#include "th.txt"
		};
	hls::stream<ap_uint<FMAP_DW> > st_layer8;
	vgg_layer<DIM_LAYER4, CH_LAYER3, CH_LAYER4>(max_pool3, k8, th8, st_layer8, rep);

	hls::stream<ap_uint<FMAP_DW> > st_layer9;
	vgg_layer<DIM_LAYER4, CH_LAYER4, CH_LAYER4>(st_layer8, k9, th9, st_layer9, rep);

	hls::stream<ap_uint<FMAP_DW> > st_layer10;
	vgg_layer<DIM_LAYER4, CH_LAYER4, CH_LAYER4>(st_layer9, k10, th10, st_layer10, rep);

	max_pool<DIM_LAYER4,CH_LAYER4,2>(st_layer10, max_pool4, rep);
		

hls::stream<ap_uint<FMAP_DW> > max_pool5;

		const T_SUM th11[(1 << FMAP_DW)-1]={
#include "th.txt"
		};
		const T_SUM th12[(1 << FMAP_DW)-1]={
#include "th.txt"
		};
		const T_SUM th13[(1 << FMAP_DW)-1]={
#include "th.txt"
		};
	hls::stream<ap_uint<FMAP_DW> > st_layer11;
	vgg_layer<DIM_LAYER5, CH_LAYER4, CH_LAYER5>(max_pool4, k11, th11, st_layer11, rep);

	hls::stream<ap_uint<FMAP_DW> > st_layer12;
	vgg_layer<DIM_LAYER5, CH_LAYER5, CH_LAYER5>(st_layer11, k12, th12, st_layer12, rep);

	hls::stream<ap_uint<FMAP_DW> > st_layer13;
	vgg_layer<DIM_LAYER5, CH_LAYER5, CH_LAYER5>(st_layer12, k13, th13, st_layer13, rep);

	max_pool<DIM_LAYER5,CH_LAYER5,2>(st_layer13, max_pool5, rep);
		


		const T_SUM th14[(1 << FMAP_DW)-1]={
#include "th.txt"
		};
		const T_SUM th15[(1 << FMAP_DW)-1]={
#include "th.txt"
		};
		const T_SUM th16[(1 << FMAP_DW)-1]={
#include "th.txt"
		};
	hls::stream<ap_uint<FMAP_DW> > st_layer14;
	fc_layer<FC_DIM0 * FC_DIM0 *CH_LAYER5, FC_LAYER1>(max_pool5, k14, th14, st_layer14,rep);

	hls::stream<ap_uint<FMAP_DW> > st_layer15;
	fc_layer<FC_LAYER1, FC_LAYER1>(st_layer14, k15, th15, st_layer15,rep);

	hls::stream<ap_uint<FMAP_DW> > st_layer16;
	fc_layer<FC_LAYER1, FC_LAYER2>(st_layer15, k16, th16, st_layer16,rep);

	static const int FACTOR = OUT_DW/FMAP_DW;
	hls::stream<ap_uint<OUT_DW> > out_layer;
	packStream<FC_LAYER2,FMAP_DW, FACTOR>(st_layer16, out_layer,rep);
	S2M<FC_LAYER2/FACTOR, ap_uint<OUT_DW>, ap_uint<OUT_DW> >(out_layer, out, rep);
		
}
