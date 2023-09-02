#pragma once
#ifndef __SYNTHESIS__
#include <cassert>
#endif
#include "ap_int.h"
#include "hls_stream.h"

//   val ethType = UInt(ETH_TYPE_BITS.W)
//   val srcmac  = UInt(ETH_MAC_BITS.W)
//   val dstmac  = UInt(ETH_MAC_BITS.W)
//   val padding = UInt(ETH_PAD_BITS.W)
//   val NET_IF_WIDTH = 64
//   val NET_IF_BYTES = NET_IF_WIDTH/8
//   val NET_LEN_BITS = 16
// 
//   val ETH_MAX_BYTES = 1520
//   val ETH_HEAD_BYTES = 16
//   val ETH_MAC_BITS = 48
//   val ETH_TYPE_BITS = 16
//   val ETH_PAD_BITS = 16

// packet_size 1-190 
// send_count is the number of packets 


template<typename T_IN> 
void recv_packets(hls::stream<T_IN >& output, int recv_count, hls::stream<ap_uint<128> >& req_head, hls::stream<ap_uint<65> >& req_data, ap_uint<64>srcmac, ap_uint<64>dstmac){
  ap_uint<64> read_head;
  ap_uint<65> recv_data;
  volatile char packet_read = 0;

  for (int i =0 ; i < recv_count; i++) {
    if (packet_read == 0){ 
      read_head = req_head.read();
      packet_read = 1;
    }   

    if (packet_read == 1){ 
      while (1) {
          recv_data = req_data.read();
          output.write((T_IN) recv_data(63, 0));
        if (recv_data.range(64, 64) == 1) {
          packet_read = 0;
          break;
        }   
      }   
    }   
  }
}

static int send_counter=0;
template<typename T_IN> 
void send_packets(hls::stream<T_IN>& input, int send_count, hls::stream<ap_uint<128> >& resp_head, hls::stream<ap_uint<65> >& resp_data, ap_uint<64>srcmac, ap_uint<64>dstmac){

  ap_uint<16> RMEM_RESP_ETH_TYPE = 0x0508L;

  ap_uint<128> resp_eth_head;
  ap_uint<128> req_eth_head;

  resp_eth_head.range(15,0) = 0;
  resp_eth_head.range(63,16) = dstmac;
  resp_eth_head.range(111,64) = srcmac;
  resp_eth_head.range(127,112) = RMEM_RESP_ETH_TYPE;
  ap_uint<65> send_data;

  int packet_size = 1;
  volatile char packet_write = 0;
  for (int i =0 ; i < send_count; i++) {

    // Send a request
    if (packet_write == 0){
      resp_head.write(resp_eth_head);
      packet_write = 1;
    }

    for(int j =0; j < packet_size; j++) {
      //send_data(63,0) = (ap_uint<64>)input.read();
      send_data(63,0) = send_counter + input.read();

      if (j == (packet_size - 1)) {
        send_data(64,64) = 1;
      } else {
        send_data(64,64) = 0;
      }

      if (packet_write == 1){
        resp_data.write(send_data);
      }
      //if (j == (packet_size - 1)) {
      //}
    }
    packet_write = 0;
  }
}

template<int DIM, int CH0, typename T> 
void recv_from_nework (hls::stream<T>& in, int REP, hls::stream<ap_uint<128> >& req_head, hls::stream<ap_uint<65> >& req_data, ap_uint<64> srcmac, ap_uint<64> dstmac){
	for(int rep = 0; rep < REP; rep++){
		for(int i=0;i<DIM;i++){
			for(int j=0;j<DIM;j++){
				for(int c=0;c<CH0;c++){
#pragma HLe PIPELINE
            //hls::stream<ap_uint<64> > output;
            T data;
            recv_packets<T>(in, 1, req_head, req_data, srcmac, dstmac);
            //data = output.read(); 
						//in.write(data);
				}
			}
    }
	}
}


template<int D, int C, int S, typename T>
void send_to_network(hls::stream<T> &out, const int REP, hls::stream<ap_uint<128> >& resp_head, hls::stream<ap_uint<65> >& resp_data, ap_uint<64>srcmac, ap_uint<64>dstmac){
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
						continue;
					}
					if( is == S - 1 && js == S -1){
            //hls::stream<ap_uint<64> > input;
						//input.write((ap_uint<64>)out.read());
            send_packets<T>(out, 1, resp_head, resp_data, srcmac, dstmac);
            send_counter ++;
          }
				}
	}
}


