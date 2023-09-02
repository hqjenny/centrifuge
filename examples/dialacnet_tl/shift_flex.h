#pragma once
#ifndef __SYNTHESIS__
#include <cassert>
#endif
#include "hls_stream.h"
#include "ap_int.h"


template<int MAX_OC, int PA, int PE, int IN_W, int W_W, int OUT_W, typename T_IN, typename T_W, typename T_OUT>
void _conv2d_flex(hls::stream<ap_uint<IN_W*PA> > &fmap, hls::stream<ap_uint<OUT_W*PE> > &out, const ap_uint<W_W*PE*PA> *p, ap_uint<8> D, int C, int OC, bool skip, ap_uint<8> batch){
//#pragma HLS INLINE

#ifndef __SYNTHESIS__
	assert(OC % PE ==0);
	assert(OC <= MAX_OC);
	assert(C % PA ==0);
#endif
	const int OCPE = OC/PE;
	static const int MOCPE = MAX_OC/PE;
	static const int MCPA = MAX_OC/PA;
	const int CPA = C/PA;

	T_OUT sum[MOCPE][PE];
#pragma HLS ARRAY_PARTITION variable=sum complete dim=2
//#pragma HLS ARRAY_PARTITION variable=sum cyclic factor=PA dim=1

	for(int n=0;n<OCPE;n++)
		for(int pe=0;pe<PE;pe++)
#pragma HLS UNROLL
		sum[n][pe] = 0;

	ap_uint<IN_W*PA> tmp; 
	for(int bat = 0; bat < batch*D*D; bat++){
		for(int c=0;c<MCPA && c<CPA;c++){
			for(int n=0;n<MOCPE && n<OCPE;n++){
				/*
					 if(c>=CPA || n >=OCPE)
					 break;
					 */
#pragma HLS PIPELINE
				if(n ==0){
					tmp = fmap.read();
					if(skip){
						out.write(tmp);
						break;
					}
				}
				ap_uint<W_W * PE * PA> w = p[c * OCPE + n];
				int w_start = 0, w_end= W_W-1;//pa*PE+pe;
				int pix_start =0, pix_end = IN_W -1;
				for(int pa=0;pa<PA;pa++){
#pragma HLS UNROLL
					ap_uint<W_W> pix = tmp.range(pix_end, pix_start);
					pix_end+=IN_W;
					pix_start+=IN_W;
					for(int pe=0;pe<PE;pe++){
#pragma HLS UNROLL
						ap_uint<W_W> part = w.range(w_end, w_start);
						sum[n][pe] += (*(T_W*)(&part)) * (*(T_IN*)(&pix));
						w_end+=W_W;
						w_start+=W_W;
					}
				}
			}
		}

		for(int n=0;n<MOCPE && n <OCPE && skip ==false;n++){
#pragma HLS PIPELINE
			ap_uint<OUT_W*PE> value=0;
			int pix_start =0, pix_end = OUT_W -1;
			for(int pe=0;pe<PE;pe++){
				value.range(pix_end, pix_start) = sum[n][pe];
				sum[n][pe] = 0;
				pix_end+=OUT_W;
				pix_start+=OUT_W;
			}
			out.write(value);
		}
	}
}


template<int MAX_OC, int PA, int PE, int IN_W, int W_W, int OUT_W, typename T_IN, typename T_W, typename T_OUT>
void _conv2d_flex(hls::stream<ap_uint<IN_W*PA> > &fmap, hls::stream<ap_uint<OUT_W*PE> > &out, const ap_uint<W_W*PE*PA> *p, ap_uint<8> D, int C, int OC, ap_uint<8> batch){
#pragma HLS INLINE
	_conv2d_flex<MAX_OC, PA, PE, IN_W, W_W, OUT_W, T_IN, T_W, T_OUT>(fmap, out, p, D, C, OC, false, batch);
}

template<int MAX_D, int MAX_C, int D_W, int PA>
void _shift_flex(hls::stream<ap_uint<D_W*PA> > &fmap, hls::stream<ap_uint<D_W*PA> > &omap, const ap_uint<8> D, const int C, const ap_uint<8> batch){
//#pragma HLS INLINE

	const int G = (C/9) * 9;
	const int CPA = C/PA;
	static const int MCPA = MAX_C/PA;
#ifndef __SYNTHESIS__
	assert(C%PA==0);
	static ap_uint<D_W*PA>  buffer[2][MAX_D+2][MCPA];
#else
	ap_uint<D_W*PA>  buffer[2][MAX_D+2][MCPA];
#endif
#pragma HLS ARRAY_PARTITION variable=buffer complete dim=1
	ap_uint<D_W*PA>  crop[3][3][MCPA];
#pragma HLS ARRAY_PARTITION variable=crop complete dim=1
#pragma HLS ARRAY_PARTITION variable=crop complete dim=2
	ap_uint<D_W*PA>  value;
	for(int bat = 0; bat < batch; bat++){
		for(int i=0, j=0;i<MAX_D + 2;j++){
			if(i>=D+2)
				break;
			if(j>=D+2){
				j=0;
				i++;
			}
			int ci = (i & 0x01);
			for(int c = 0;c<CPA;c++){
#pragma HLS PIPELINE
				crop[0][0][c] = crop[0][1][c];
				crop[0][1][c] = crop[0][2][c];
				crop[1][0][c] = crop[1][1][c];
				crop[1][1][c] = crop[1][2][c];
				crop[2][0][c] = crop[2][1][c];
				crop[2][1][c] = crop[2][2][c];
				crop[0][2][c] = buffer[ci][j][c];
				crop[1][2][c] = buffer[ci^0x01][j][c];
				// read from fmap and put to crop and buffer

				if(i==0 || i == D + 1 || j==0 || j == D+1){
					buffer[ci][j][c] = 0;
					crop[2][2][c] = 0;
				}
				else {
					ap_uint<D_W*PA> r = fmap.read();
					crop[2][2][c] = r;
					buffer[ci][j][c] = r;
				}
				ap_uint<D_W*PA> value=0;
				for(int pa=0;pa<PA;pa++){
					int c_index = c*PA+pa;
					if(c_index>=G)
						value.range(D_W*(pa+1)-1, D_W*pa) = crop[1][1][c].range(D_W*(pa+1)-1, D_W*pa);
					else{
						int d_x = c%3;
						int d_y = (c/3)%3;
						value.range(D_W*(pa+1)-1, D_W*pa) = crop[d_y][d_x][c].range(D_W*(pa+1)-1, D_W*pa);
					}
				}
				if(j>=2 && i>=2)
					omap.write(value);
			}
		}
	}
}
template<int MAX_D, int MAX_C, typename T>
void _shift_reg(hls::stream<T> &fmap, hls::stream<T> &omap, const ap_uint<8> D, const int C, const ap_uint<8> batch){
//#pragma HLS INLINE

	const int G = (C/9) * 9;
#ifndef __SYNTHESIS__
	static T buffer[2][MAX_D+2][MAX_C];
#else
	T buffer[2][MAX_D+2][MAX_C];
#endif
#pragma HLS ARRAY_PARTITION variable=buffer complete dim=1
	T crop[3][3][MAX_C];
#pragma HLS ARRAY_PARTITION variable=crop complete dim=1
#pragma HLS ARRAY_PARTITION variable=crop complete dim=2
	T value;
	int d_x, d_y=0;
	for(int bat = 0; bat < batch; bat++){
		for(int i=0;i<D + 2;i++){
			for(int j=0;j<D + 2;j++){
				int ci = (i & 0x01);
				for(int c = 0, d_x=0;c<C;c++, d_x++){
#pragma HLS PIPELINE
					/*
					// crop shift left
					for(int si = 0; si <3;si++)
					for(int sj = 0; sj <2;sj++)
					crop[si][sj][c] = crop[si][sj+1][c];
					// crop read buffer
					for(int si = 0; si < 2;si++){
					int bi = (ci + si) & 0x01;
					crop[si][2][c] = buffer[bi][j][c];
					}
					*/
					crop[0][0][c] = crop[0][1][c];
					crop[0][1][c] = crop[0][2][c];
					crop[1][0][c] = crop[1][1][c];
					crop[1][1][c] = crop[1][2][c];
					crop[2][0][c] = crop[2][1][c];
					crop[2][1][c] = crop[2][2][c];
					crop[0][2][c] = buffer[ci][j][c];
					crop[1][2][c] = buffer[ci^0x01][j][c];
					// read from fmap and put to crop and buffer

					if(i==0 || i == D + 1 || j==0 || j == D+1){
						buffer[ci][j][c] = 0;
						crop[2][2][c] = 0;
					}
					else {
						T r = fmap.read();
						crop[2][2][c] = r;
						buffer[ci][j][c] = r;
					}
					if(c>=G)
						value = crop[1][1][c];
					else{
						if(d_x == 3){
							d_y ++;
							d_x =0;
						}
						if(d_y == 3)
							d_y =0;
						value = crop[d_y][d_x][c];
					}
					if(j>=2 && i>=2)
						omap.write(value);
				}
			}
		}
	}
}
template<int IN_W, int ACT_W, int PA, typename T_IN>
void _relu_flex(hls::stream<ap_uint<IN_W*PA> > &fmap, hls::stream<ap_uint<ACT_W * PA> > &out, const T_IN thresholds[(1<<ACT_W) - 1], const ap_uint<8> D, const int C, bool skip, const ap_uint<8> batch){
//#pragma HLS INLINE
#ifndef __SYNTHESIS__
	assert(C % PA ==0);
#endif
#pragma HLS ARRAY_PARTITION variable=thresholds complete dim=1

	const int CPA = C/PA;
	const int NUM_S = (1 << ACT_W) - 1;
	for(int bat = 0; bat < batch * D * D; bat++){
		for(int k=0;k<CPA;k++){
#pragma HLS PIPELINE
			ap_uint<IN_W*PA> r = fmap.read();
			if(skip){
				out.write(r);
				continue;
			}
			ap_uint<ACT_W*PA> value= 0;
			for(int pa=0;pa<PA;pa++){
				ap_uint<IN_W> part = r.range(IN_W*(pa+1)-1, IN_W*pa);
				T_IN com = *(T_IN*)(&part);
				int index = 0;
				for(int i=0;i<NUM_S;i++){
					if(com < thresholds[i]){
						index = i;
						break;
					}
					if(i==NUM_S - 1)
						index = NUM_S;
				}
				value.range(ACT_W*(pa+1)-1, ACT_W*pa) = index;
			}
			out.write(value);
		}
	}
}

template<int IN_W, int ACT_W, int PA, typename T_IN>
void _relu_flex(hls::stream<ap_uint<IN_W*PA> > &fmap, hls::stream<ap_uint<ACT_W * PA> > &out, const T_IN thresholds[(1<<ACT_W) - 1], const ap_uint<8> D, const int C, const ap_uint<8> batch){
#pragma HLS INLINE
	_relu_flex<IN_W, ACT_W, PA, T_IN>(fmap, out, thresholds, D, C, false, batch);
}

template<int D, int C, int S, typename T>
void _max_pool(hls::stream<T> &fmap, hls::stream<T> &out, const ap_uint<8> batch){
//#pragma HLS INLINE
	static const int nD = D/S;
	T buffer[nD][C];
	for(int bat = 0; bat < batch; bat++){
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


template<int MAX_D, int MAX_C, typename T>
void _max_pool_2x2_reg(hls::stream<T> &fmap, hls::stream<T> &out, const ap_uint<8> D, const int C, bool skip, const ap_uint<8> batch){
//#pragma HLS INLINE
#ifndef __SYNTHESIS__
	assert(D %2 ==0);
#endif
	static const int nD = MAX_D>>1;
#ifndef __SYNTHESIS__
	static T buffer[nD][MAX_C];
#else
	T buffer[nD][MAX_C];
#endif
	for(int bat = 0; bat < batch; bat++){
		int c =0, is =0, js =0;
		for(int i = 0;i<D;i++, is++)
			for(int j = 0, js = 0, c = 0;j<D;j++, js++)
				for(int k = 0;k<C;k++){
					if(skip){
						out.write(fmap.read());
						continue;
					}
#pragma HLS PIPELINE
					if(is == 2) is = 0;
					if(js == 2) {
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
					if( is == 1 && js == 1)
						out.write(cmp);
				}
	}
}

template<int MAX_D, int MAX_C, int D_W, int PA>
void _max_pool_2x2(hls::stream<ap_uint<D_W*PA> > &fmap, hls::stream<ap_uint<D_W*PA> > &out, const ap_uint<8> D, const int C, bool skip, const ap_uint<8> batch){
//#pragma HLS INLINE
#ifndef __SYNTHESIS__
	assert(D %2 ==0);
	assert(C%PA==0);
#endif
	static const int nD = MAX_D>>1;
	static const int MCPA = MAX_C/PA;
	const int CPA = C/PA;
#ifndef __SYNTHESIS__
	static ap_uint<D_W*PA> buffer[nD][MCPA];
#else
	ap_uint<D_W*PA> buffer[nD][MCPA];
#endif
	for(int bat = 0; bat < batch; bat++){
		int c =0, is =0, js =0;
		for(int i = 0;i<D;i++, is++)
			for(int j = 0, js = 0, c = 0;j<D;j++, js++)
				for(int k = 0;k<CPA;k++){
					if(skip){
						out.write(fmap.read());
						continue;
					}
#pragma HLS PIPELINE
					if(is == 2) is = 0;
					if(js == 2) {
						js = 0;
						c++;
					}
					ap_uint<D_W*PA> r = fmap.read();
					ap_uint<D_W*PA> cmp = buffer[c][k];
					ap_uint<D_W*PA> ncmp;
					for(int pa=0;pa<PA;pa++){
						ap_uint<D_W> rp = r.range(D_W*(pa+1)-1, D_W*pa);
						ap_uint<D_W> cp = cmp.range(D_W*(pa+1)-1, D_W*pa);
						ncmp.range(D_W*(pa+1)-1, D_W*pa) = cp>rp?cp:rp;
					}
					if(is == 0 && js ==0)
						buffer[c][k] = r;
					else if( is == 1 && js == 1)
						out.write(ncmp);
					else
						buffer[c][k] = ncmp;

				}
	}
}
