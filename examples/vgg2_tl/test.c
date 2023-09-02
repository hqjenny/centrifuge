int main(){

    int D = 16;
    int C = 32;
    int S = 2;
	  int nD = D/S;
    int send_count = 0;
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
            //send_packets<T>(out, 1, resp_head, resp_data, srcmac, dstmac);
            send_count++;
          }
				}

  printf("%d", send_count);
  return 0;
}

