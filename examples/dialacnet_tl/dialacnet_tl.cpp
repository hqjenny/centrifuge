#include "ap_int.h"
#include "hls_stream.h"
#include "shift_flex.h"
#include "dma.h"
#include "para.h"


template<int MAX_D, int MAX_C, int PA, int PE>
void shiftLayer_FIXED(hls::stream<ap_uint<FM_W*PA> > &fmap, 
		hls::stream<ap_uint<FM_W*PA> > &out, 
		const ap_uint<W_W*PA*PE> *k0,
		const ap_uint<W_W*PA*PE> *k1,
		const T_SUM *th0,
		const T_SUM *th1,
		const int FM_D,
		const int IN_CH,
		bool skip_maxPool,
		int batch){
#pragma HLS INLINE
	const int MID_CH = skip_maxPool?IN_CH:IN_CH << 1;
	const int MID_D = skip_maxPool? FM_D : FM_D>>1;
	hls::stream<ap_uint<SUM_W*PE> > s_conv0;
#pragma HLS STREAM variable=s_conv0 depth=16 dim=1
	hls::stream<ap_uint<FM_W*PE> > s_relu0;
#pragma HLS STREAM variable=s_relu0 depth=16 dim=1
	_conv2d_flex<MAX_C, PA, PE, FM_W, W_W, SUM_W, T_FMAP, T_W, T_SUM>(fmap, s_conv0, k0, FM_D, IN_CH, MID_CH, batch);
	_relu_flex<SUM_W, FM_W, PE, T_SUM>(s_conv0, s_relu0, th0, FM_D, MID_CH, batch);

	hls::stream<ap_uint<FM_W*PE> > s_pool, s_shift;
#pragma HLS STREAM variable=s_pool depth=16 dim=1
	_max_pool_2x2<MAX_D, MAX_C, FM_W, PE>(s_relu0, s_pool, FM_D, MID_CH, skip_maxPool, batch);

	_shift_flex<MAX_D, MAX_C, FM_W, PE>(s_pool, s_shift, MID_D, MID_CH, batch);
	
	hls::stream<ap_uint<SUM_W*PA> > s_conv1;
#pragma HLS STREAM variable=s_conv1 depth=16 dim=1
	_conv2d_flex<MAX_C, PE, PA, FM_W, W_W, SUM_W, T_FMAP, T_W, T_SUM>(s_shift, s_conv1, k1, MID_D, MID_CH, MID_CH, batch);
	_relu_flex<SUM_W, FM_W, PA, T_SUM>(s_conv1, out, th1, MID_D, MID_CH, batch);
}

template<int MAX_D, int MAX_C, int PE>
void shiftLayer_RES(hls::stream<ap_uint<FM_W*PE> > &fmap, 
		hls::stream<ap_uint<FM_W*PE> > &out, 
		const ap_uint<W_W*PE*PE> *k2,
		const T_SUM *th0,
		const int FM_D,
		const int IN_CH,
		bool skip,
		int batch){
#pragma HLS INLINE
	const int MID_CH = skip?IN_CH:IN_CH << 1;
	const int MID_D = skip? FM_D : FM_D>>1;

	hls::stream<ap_uint<FM_W*PE> > s_pool;
#pragma HLS STREAM variable=s_pool depth=16 dim=1
	_max_pool_2x2<MAX_D, MAX_C, FM_W, PE>(fmap, s_pool, FM_D, IN_CH, skip, batch);
	
	hls::stream<ap_uint<SUM_W*PE> > s_conv0;
#pragma HLS STREAM variable=s_conv0 depth=16 dim=1
	_conv2d_flex<MAX_C, PE, PE, FM_W, W_W, SUM_W, T_FMAP, T_W, T_SUM>(s_pool, s_conv0, k2, MID_D, IN_CH, MID_CH, skip, batch);
	_relu_flex<SUM_W, FM_W, PE, T_SUM>(s_conv0, out, th0, MID_D, MID_CH, skip, batch);
}


void wrapper(ap_uint<FM_W*PA_0>* fmap, ap_uint<FM_W*PA_0> * out,
		ap_uint<W_W * PE_0 * PA_0> *k0,
		ap_uint<W_W * PE_0 * PA_0> *k1,
		ap_uint<W_W * PA_0 * PA_0> *k2,
		int FM_D, 
		int FM_CH,
		int th_i,
		bool pool,
		int batch){
//#pragma HLS INLINE 
#pragma HLS INLINE off

	const int MAX_LAYERS = 16;
	const int MAX_D = 224;
	const int MAX_CH = 1024;
	const int MID_CH = pool? FM_CH<<1:FM_CH;
	const int MID_D = pool? FM_D>>1 : FM_D;

#pragma HLS DATAFLOW

	const T_SUM th0[MAX_LAYERS][(1<<FM_W)-1]={
#include "th.txt"
	};
	const T_SUM th1[MAX_LAYERS][(1<<FM_W)-1]={
#include "th.txt"
	};
	const T_SUM th2[MAX_LAYERS][(1<<FM_W)-1]={
#include "th.txt"
	};
	hls::stream<ap_uint<FM_W*PA_0> > st_layer0; 
#pragma HLS STREAM variable=st_layer0 depth=16 dim=1
	hls::stream<ap_uint<FM_W*PA_0> > out_layer;
#pragma HLS STREAM variable=out_layer depth=16 dim=1
	M2S<ap_uint<FM_W*PA_0>, ap_uint<FM_W*PA_0> >(fmap, st_layer0, FM_D*FM_D*FM_CH/PA_0*batch);
	hls::stream<ap_uint<FM_W*PA_0> > out_left, out_right;
#pragma HLS STREAM variable=out_left depth=16 dim=1
#pragma HLS STREAM variable=out_right depth=16 dim=1
	hls::stream<ap_uint<FM_W*PA_0> > left, right;
#pragma HLS STREAM variable=left depth=16 dim=1
#pragma HLS STREAM variable=right depth=4*16*256/32 dim=1
	splitStream(st_layer0, left, right, FM_CH/PA_0, FM_D*FM_D*batch);

	// shift-layer
	shiftLayer_FIXED<MAX_D, MAX_CH, PA_0, PE_0>(left,out_left, 
			k0, k1, th0[th_i], th1[th_i],
			FM_D,	FM_CH>>1, !pool,batch);
	shiftLayer_RES<MAX_D, MAX_CH, PA_0>(right,out_right, 
			k2, th2[th_i],
			FM_D,	FM_CH>>1, !pool,batch);

	mergeStream(out_left, out_right, out_layer, MID_CH/PA_0, MID_D*MID_D*batch);
//template<typename T_OUT, typename T_IN>
//void S2M(hls::stream<T_IN> &s_mem, T_OUT *mem, int REP){
//
	S2M<ap_uint<FM_W*PA_0>, ap_uint<FM_W*PA_0>>(out_layer, out, MID_D*MID_D*MID_CH/PA_0*batch);
}


extern "C"
void top(ap_uint<FM_W*PA_0>* fmap, ap_uint<FM_W*PA_0> * out,
		ap_uint<W_W * PE_0 * PA_0> *k0,
		ap_uint<W_W * PE_0 * PA_0> *k1,
		ap_uint<W_W * PA_0 * PA_0> *k2,
		int FM_D, 
		int FM_CH,
		int th_i,
		bool pool,
		int batch
		){
#pragma HLS INTERFACE m_axi port=fmap offset=slave bundle=gmem depth=4*32*32*128/32
#pragma HLS INTERFACE s_axilite port=fmap bundle=control 
#pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem3 depth=4*32*32*128/32
#pragma HLS INTERFACE s_axilite port=out bundle=control

#pragma HLS INTERFACE m_axi port=k0 bundle=gmem0 depth=128*128/32/32
#pragma HLS INTERFACE s_axilite port=k0 bundle=control
#pragma HLS INTERFACE m_axi port=k1 bundle=gmem1 depth=128*128/32/32
#pragma HLS INTERFACE s_axilite port=k1 bundle=control
#pragma HLS INTERFACE m_axi port=k2 bundle=gmem2 depth=128*128/32/32
#pragma HLS INTERFACE s_axilite port=k2 bundle=control

#pragma HLS INTERFACE s_axilite port=FM_D bundle=control
#pragma HLS INTERFACE s_axilite port=FM_CH bundle=control
#pragma HLS INTERFACE s_axilite port=th_i bundle=control
#pragma HLS INTERFACE s_axilite port=pool bundle=control
#pragma HLS INTERFACE s_axilite port=batch bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

	const int MAX_LAYERS = 16;
	const int MAX_D = 224;
	const int MAX_CH = 1024;
	const int MID_CH = pool? FM_CH<<1:FM_CH;
	const int MID_D = pool? FM_D>>1 : FM_D;

  ap_uint<W_W * PE_0 * PA_0> k0_buffer[512 * 512 / PE_0 * PA_0];
  ap_uint<W_W * PE_0 * PA_0> k1_buffer[512 * 512 / PE_0 * PA_0];
  ap_uint<W_W * PE_0 * PA_0> k2_buffer[512 * 512 / PE_0 * PA_0];
  
  int C = FM_CH>>1;
  static const int MOCPE = MAX_CH/PE_0;
  static const int MCPA = MAX_CH/PA_0;
  const int CPA = C/PA_0;
  const int OCPE = (MID_CH >> 1)/PE_0;

  for(int c=0;c<MCPA && c<CPA;c++){
    for(int n=0;n<MOCPE && n<OCPE;n++){
				ap_uint<W_W * PE_0 * PA_0> k0_buffer = k0[c * OCPE + n];
    }
  }
  for(int c=0;c<MCPA && c<CPA;c++){
    for(int n=0;n<MOCPE && n<OCPE;n++){
				ap_uint<W_W * PE_0 * PA_0> k1_buffer = k1[c * OCPE + n];
    }
  }

  const int MOCPE2 = MAX_CH/PA_0;
  const int OCPE2=(MID_CH >> 1)/PA_0;
  for(int c=0;c<MCPA && c<CPA;c++){
    for(int n=0;n<MOCPE2 && n<OCPE2;n++){
				ap_uint<W_W * PE_0 * PA_0> k2_buffer = k2[c * OCPE + n];
    }
  }
	wrapper(fmap,out,k0_buffer,k1_buffer,k2_buffer,FM_D, FM_CH, th_i, pool, batch);
}

