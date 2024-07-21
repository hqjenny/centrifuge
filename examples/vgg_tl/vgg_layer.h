template<int DIM, int CH0, int CH1, typename T_IN, typename T_OUT>
void vgg_layer(hls::stream<T_IN> &st_layer, 
		const ap_uint<WEIGHT_DW * PE> k[3][3][CH0][CH1/PE],
		const T_SUM thr[(1 << FMAP_DW)],
		hls::stream<T_OUT> &out_layer, int rep){
#pragma HLS INLINE
	hls::stream<T_IN> pad_layer;
	hls::stream<T_SUM> conv_layer;
	padding<T_IN>(st_layer, pad_layer,DIM, CH0, 1, rep);
	conv2d<DIM+2, CH0, CH1, PE, WEIGHT_DW, T_W, T_IN, T_SUM>(pad_layer, k, conv_layer, rep);
	relu_c<(1 << FMAP_DW), T_SUM, T_OUT>(conv_layer, out_layer, thr, DIM*DIM*CH1*rep);
}

template<int M, int N, typename T_IN, typename T_OUT>
void fc_layer(hls::stream<T_IN> &st_layer, 
		const ap_uint<WEIGHT_DW * PE> k[M][N/PE],
		const T_SUM thr[(1 << FMAP_DW)],
		hls::stream<T_OUT> &out_layer, int rep){
#pragma HLS INLINE
	hls::stream<T_SUM> fc_layer;
	matMul<M, N, PE, WEIGHT_DW, T_W>(st_layer, fc_layer, k, rep);
	relu_c<(1 << FMAP_DW), T_SUM, T_OUT>(fc_layer, out_layer, thr, rep*N);
}

