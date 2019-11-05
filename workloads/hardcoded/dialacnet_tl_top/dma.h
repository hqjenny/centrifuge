#pragma once
#ifndef __SYNTHESIS__
#include <cassert>
#endif

// M2S
template<int M, int C, int REP, typename T_IN, typename T_OUT>
void M2S(T_IN *mem, hls::stream<T_OUT> s_mem[C]){
//#pragma HLS INLINE
	for(int rep = 0; rep < REP; rep++)
		for(int i=0;i<M;i++)
			for(int c=0;c<C;c++){
#pragma HLS pipeline
				s_mem[c].write((T_OUT)mem[rep*M*C + i*C+c]);
			}
}



// S2M
template<int M, int C, int REP, typename T_IN, typename T_OUT>
void S2M(hls::stream<T_IN> s_mem[C], T_OUT *mem){
//#pragma HLS INLINE
	for(int rep = 0; rep < REP; rep++)
		for(int i=0;i<M;i++)
			for(int c=0;c<C;c++)
#pragma HLS pipeline
				mem[rep * M *C + i*C + c]= (T_OUT)s_mem[c].read();
}


template<typename T_IN, typename T_OUT>
void M2S(T_IN *mem, hls::stream<T_OUT> &s_mem, int REP){
//#pragma HLS INLINE
	int index = 0;
	for(int rep = 0; rep < REP; rep++){
#pragma HLS pipeline
		s_mem.write((T_OUT)mem[index]);
		index ++;
	}
}



// S2M
template<typename T_OUT, typename T_IN>
void S2M(hls::stream<T_IN> &s_mem, T_OUT *mem, int REP){
//#pragma HLS INLINE
	int index = 0;
	for(int rep = 0; rep < REP; rep++){
#pragma HLS pipeline
		mem[index]= (T_OUT)s_mem.read();
		index ++;
	}
}



template<int DW, int PACK>
void unPackStream(hls::stream<ap_uint<DW * PACK> > &in, hls::stream<ap_uint<DW> >&out, int REP){
	for(int rep = 0; rep < REP; rep++){
		ap_uint<DW*PACK> value = in.read();
		int begin = DW -1, end  = 0;
		for(int c=0;c<PACK;c++){
#pragma HLS pipeline
			ap_uint<DW> r = value(begin, end);
			out.write(r);
			begin+=DW;
			end+=DW;
		}
	}

}
template<int DW, int PACK>
void packStream(hls::stream<ap_uint<DW> > &in, hls::stream<ap_uint<DW*PACK> >&out, int REP){
#ifndef __SYNTHESIS__
	assert(REP%PACK==0);
#endif
	const int Iter = REP/PACK; 
	for(int rep = 0; rep < Iter; rep++){
		ap_uint<DW*PACK> value;
		int begin = DW -1, end  = 0;
		for(int c=0;c<PACK;c++){
#pragma HLS pipeline
			ap_uint<DW> r = in.read();
			value(begin, end) = r;
			begin+=DW;
			end+=DW;
		}
		out.write(value);
	}
}

template<typename T>
void splitStream(hls::stream<T> & in, hls::stream<T> &out0, hls::stream<T> &out1, int CPA, int REP){
//#pragma HLS INLINE
#ifndef __SYNTHESIS__
	assert(CPA % 2 ==0);
#endif
	for(int rep = 0; rep < REP; rep++)
		for(int cpa=0;cpa<CPA;cpa++)
#pragma HLS pipeline
			if(cpa < (CPA >> 1))
				out0.write(in.read());
			else
				out1.write(in.read());
}



template<typename T>
void mergeStream(hls::stream<T> &in0, hls::stream<T> &in1, hls::stream<T> &out,int CPA,  int REP){
//#pragma HLS INLINE
#ifndef __SYNTHESIS__
	assert(CPA % 2 ==0);
#endif
	for(int rep = 0; rep < REP; rep++)
		for(int cpa=0;cpa<(CPA>>1);cpa++){
#pragma HLS pipeline II=2
			T c0 = in0.read();
			T c1 = in1.read();
			out.write(c0);
			out.write(c1);
		}
}
