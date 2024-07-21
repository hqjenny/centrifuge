#pragma once
#ifndef __SYNTHESIS__
#include <cassert>
#endif
#include "ap_int.h"
#include "hls_stream.h"

template<typename T>
void padding(hls::stream<T> &fmap, hls::stream<T> &omap, int D, int C, int P, int REP){

	for(int rep = 0; rep < REP; rep++){
		for(int i=0;i<D + 2*P;i++)
			for(int j=0;j<D + 2*P;j++){
				bool pad = (i < P) || (i >= D + P) || (j < P) || (j >= D + P);
				for(int c=0;c<C;c++){
#pragma HLS PIPELINE
					if(pad)
						omap.write(0);
					else
						omap.write(fmap.read());
				}
			}
	}
}


template< int D, int C, int OC, int PE, int W_W, typename T_W, typename T_IN, typename T_OUT>
void conv2d(hls::stream<T_IN> &fmap, const ap_uint<W_W * PE> kernel[3][3][C][OC/PE], hls::stream<T_OUT> &omap,const int REP){
#ifndef __SYNTHESIS__
	assert(OC % PE ==0);
#endif
	T_IN buffer[2][D][C];
#pragma HLS ARRAY_PARTITION variable=buffer complete dim=1
	T_IN crop[3][3][C];
#pragma HLS ARRAY_PARTITION variable=crop complete dim=1
#pragma HLS ARRAY_PARTITION variable=crop complete dim=2
	T_OUT sum[OC/PE][PE]={0};
#pragma HLS ARRAY_PARTITION variable=sum complete dim=2

	for(int rep = 0; rep < REP; rep++){
		for(int i=0;i<D;i++){
			for(int j=0;j<D;j++){
				int ci = (i & 0x01);
				bool b_out = (i >=2) && (j >=2);
				for(int c = 0;c<C;c++){
					T_IN read = fmap.read();
#pragma HLS PIPELINE
					crop[0][0][c] = crop[0][1][c];
					crop[0][1][c] = crop[0][2][c];
					crop[1][0][c] = crop[1][1][c];
					crop[1][1][c] = crop[1][2][c];
					crop[2][0][c] = crop[2][1][c];
					crop[2][1][c] = crop[2][2][c];
					crop[0][2][c] = buffer[ci][j][c];
					crop[1][2][c] = buffer[ci^0x01][j][c];
					crop[2][2][c] = read;
					buffer[ci][j][c] = read;
				}
				for(int c = 0;c<C;c++){
					for(int fi = 0; fi<3;fi++)
						for(int fj = 0; fj<3;fj++){
							for(int k=0;k<OC/PE;k++){
#pragma HLS PIPELINE
								ap_uint<W_W*PE> w = kernel[fi][fj][c][k];
								for(int pe=0;pe<PE;pe++){
#pragma HLS UNROLL
									ap_uint<W_W> w_p = w.range(W_W*(pe+1) -1, W_W*pe);
									sum[k][pe]+= crop[fi][fj][c] * (*(T_W*)(&w_p));
								}
							}
						}
				}
				for(int k=0;k<OC/PE;k++)
#pragma HLS PIPELINE
					for(int pe=0;pe<PE;pe++){
						if(b_out){
							omap.write(sum[k][pe]);
						}
						sum[k][pe] = 0;
					}
			}
		}
	}
}

template<int NUM_S, typename T_IN, typename T_OUT>
void relu_c(hls::stream<T_IN> &fmap, hls::stream<T_OUT> &out, const T_IN thresholds[NUM_S - 1], const int REP){
#pragma HLS ARRAY_PARTITION variable=thresholds complete dim=1
	for(int rep = 0; rep < REP; rep++){
#pragma HLS PIPELINE
		T_IN r = fmap.read();
		for(int i=0;i<NUM_S-1;i++){
#pragma HLS UNROLL
			if(r < thresholds[i]){
				out.write(i);
				break;
			}
			if(i==NUM_S - 2)
				out.write(NUM_S - 1);
		}		
	}
}

template< int D, int C, typename T_IN, typename T_OUT>
void relu_b(hls::stream<T_IN> &fmap, hls::stream<T_OUT> &out, const T_IN bias[C], const int REP){
	for(int rep = 0; rep < REP; rep++){
		for(int i=0;i<D;i++){
			for(int j=0;j<D;j++){
				for(int k=0;k<C;k++){
#pragma HLS PIPELINE
					T_IN r = fmap.read();
					r+=bias[k];
					out.write(r>0?r:0);
				}
			}
		}
	}
}

template<int D, int C, int S, typename T>
void max_pool(hls::stream<T> &fmap, hls::stream<T> &out, const int REP){
	static const int nD = D/S;
	T buffer[nD][C];
	for(int rep = 0; rep < REP; rep++){
		int c =0, is =0, js =0;
		for(int i = 0;i<D;i++, is++)
			for(int j = 0, js = 0, c = 0;j<D;j++, js++)
				for(int k = 0;k<C;k++){
#pragma HLS PIPELINE
					if(is == S) is = 0;
					if(js == S) {
						js = 0;
						c++;
					}
					if(c == nD){
						fmap.read();
						continue;
					}
					T r = fmap.read();
					T cmp = buffer[c][k];
					if((is == 0 && js ==0) || cmp < r){
						buffer[c][k] = r;
						cmp = r;
					}
					if( is == S - 1 && js == S -1)
						out.write(cmp);
				}
	}
}

template< int M, int N, int PE, int W_W, typename T_W,typename T_IN, typename T_OUT>
void matMul(hls::stream<T_IN> &fmap, hls::stream<T_OUT> &out, const ap_uint<W_W*PE> p[M][N/PE], int REP){
	T_OUT sum[N]={0};
#pragma HLS ARRAY_PARTITION variable=sum cyclic factor=PE dim=1

	for(int rep = 0; rep < REP; rep++){
		for(int i=0;i<M;i++){
			T_IN r = fmap.read();
			for(int n=0;n<N/PE + 1;n++){
#pragma HLS PIPELINE
				ap_uint<W_W*PE> w = p[i][n];
				for(int pe=0;pe<PE;pe++){
#pragma HLS UNROLL
					ap_uint<W_W> part = w.range(W_W*(1+pe)-1, W_W*pe);
					sum[n*PE+pe] += (*((T_W*)(&part))) * r;
				}
			}
		}
		for(int n=0;n<N;n++){
#pragma HLS UNROLL
			out.write(sum[n]);
			sum[n] = 0;
		}
	}
}
