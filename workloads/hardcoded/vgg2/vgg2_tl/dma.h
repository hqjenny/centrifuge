#pragma once
#include <cassert>

// M2S
template<int M, int C, int REP, typename T_IN, typename T_OUT>
void M2S(const T_IN *mem, hls::stream<T_OUT> s_mem[C]){
#pragma HLS INLINE
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
#pragma HLS INLINE
	for(int rep = 0; rep < REP; rep++)
		for(int i=0;i<M;i++)
			for(int c=0;c<C;c++)
#pragma HLS pipeline
				mem[rep * M *C + i*C + c]= (T_OUT)s_mem[c].read();
}


template<int M, typename T_IN, typename T_OUT>
void M2S(const T_IN *mem, hls::stream<T_OUT> &s_mem, const int REP){
#pragma HLS INLINE
	int index = 0;
	for(int rep = 0; rep < REP; rep++)
		for(int i=0;i<M;i++){
#pragma HLS pipeline
			s_mem.write((T_OUT)mem[index]);
			index ++;
		}
}



// S2M
template<int M, typename T_IN, typename T_OUT>
void S2M(hls::stream<T_IN> &s_mem, T_OUT *mem, const int REP){
#pragma HLS INLINE
	int index = 0;
	for(int rep = 0; rep < REP; rep++)
		for(int i=0;i<M;i++){
#pragma HLS pipeline
			mem[index]= (T_OUT)s_mem.read();
			index ++;
		}
}



template<int M, int DW, int PACK>
void packStream(hls::stream<ap_uint<DW> > &in, hls::stream<ap_uint<DW*PACK> >&out, const int REP){
	assert(M%PACK==0);
	static const int Iter = M/PACK; 
	for(int rep = 0; rep < REP; rep++)
		for(int i=0;i<Iter;i++){
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
