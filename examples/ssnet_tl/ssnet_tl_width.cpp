#include "shiftshuffle_kernel.h"
using namespace std;
//#define TOTAL_OC_HEIGHT (TOTAL_OC / OC) * HEIGHT


// 128=act_bit*WIDTH
// for now this is dedicated to cifar10
// fetch_in_map() fetches in fmap for a whole layer (TOTAL_IC*HEIGHT*WIDTH) for TOTAL_OC/OC times
// each cycle 128 bit(a row) is loaded
void fetch_in_fmap(ap_uint<128>* in_fmap, ap_uint<36> in_fmap_bram[128*IC/N/ACT_BIT/IC_VEC][IC_VEC], int height, int total_ic, int h_iter, int ic_t_iter, int shuffle_index){
#pragma HLS ARRAY_PARTITION variable=in_fmap_bram cyclic factor=4 dim=1
#pragma HLS ARRAY_PARTITION variable=in_fmap_bram complete dim=2

    //for(int i=0; i<(ic/ic_vec); i++){
        //for(int j=0; j<ic_vec; j++){
        for(int l=0; l<IC; l++){
#pragma HLS PIPELINE
            int i=l/IC_VEC;
            int j=l%IC_VEC;
            
            ap_uint<128> in_fmap_vec;
            int channel_index=l+ic_t_iter*IC;
            int in_fmap_addr=(h_iter*total_ic*IC+channel_index);
            //if(!is_shift){
                in_fmap_vec = in_fmap[in_fmap_addr];
            //}
            /*else{
                int shift_index=channel_index%5;
                if(shift_index==0){
                    //shift down
                    //consider index overflow
                    if(h_iter==0)
                        in_fmap_vec=0;
                    else
                        in_fmap_vec = in_fmap[in_fmap_addr-total_ic];
                }
                else if(shift_index==1){
                    //shift right
                    ap_uint<128> tmp_vec = in_fmap[in_fmap_addr];
                    in_fmap_vec.range(3,0)=0;
                    in_fmap_vec.range(127,4)=tmp_vec.range(123,0);
                }
                else if(shift_index==2){
                    //identity
                    in_fmap_vec = in_fmap[in_fmap_addr];
                }
                else if(shift_index==3){
                    //shift left
                    ap_uint<128> tmp_vec = in_fmap[in_fmap_addr];
                    in_fmap_vec.range(127,124)=0;
                    in_fmap_vec.range(123,0)=tmp_vec.range(127,4); 
                }
                else{
                    //shift up
                    //consider index overflow
                    if(h_iter==(height-1))
                        in_fmap_vec=0;
                    else
                        in_fmap_vec = in_fmap[in_fmap_addr+total_ic];
                }
            }*/
            
            for(int k=0; k<(128/N/ACT_BIT); k++){
                ap_uint<36> tmp_vec;
                tmp_vec.range(35,32)=0;
                tmp_vec.range(31,0)=in_fmap_vec.range(k*N*ACT_BIT+N*ACT_BIT-1,k*N*ACT_BIT);
                in_fmap_bram[i+k*(IC/IC_VEC)][j]=tmp_vec;
            }
        }
    //}    
}

// 324=weight_bit*OC*IC
// each cycle OC*IC is loaded
// fetch_weight() fetches weight (TOTAL_IC*TOTAL_OC)
// main function call this for  times
//void fetch_weight(ap_uint<512>* weight,
void fetch_weight(ap_uint<256>* weight1,
ap_uint<256>* weight2,
		hls::stream<ap_uint<324> >& weight_stream, int total_ic,
		int total_oc, int height) {
	//could be flattened
	for (int i = 0; i < (total_oc); i++) {
		for (int k = 0; k < (total_ic); k++) {
      ap_uint<324> data;
      data.range(255,0) = weight1[k + i * (total_ic)];
      data.range(323,256) = weight2[k + i * (total_ic)];
			weight_stream.write(
        data);
										//weight1[k + i * (total_ic)].range(323, 0));
		}
	}

}

// 128=act_bit*WIDTH
// for now this is dedicated to cifar10
// 32=N*ACT_BIT=36
// convert the in fmap stream into array
// for convolution, every cycle a entry in the array (N*act_bit) is accessed
// main function call this in double buffering manner
// after calling in_fmap_S2BRAM once, it can provide in fmap for several call on conv kernel
// ap_uint<36> in_fmap_bram[128*ic/N/act_bit/ic_vec][ic_vec]
/*void in_fmap_S2BRAM(hls::stream<ap_uint<128> >& in_fmap_stream,
		ap_uint<36> in_fmap_bram[128 * IC / N / ACT_BIT / IC_VEC][IC_VEC],
		int my_N, int act_bit, int ic, int ic_vec) {
	for (int i = 0; i < (ic / ic_vec); i++) {
		for (int j = 0; j < ic_vec; j++) {
			ap_uint<128> in_fmap_vec = in_fmap_stream.read();
			for (int k = 0; k < (128 / my_N / act_bit); k++) {
				ap_uint<36> tmp_vec;
				tmp_vec.range(35, 32) = 0;
				tmp_vec.range(31, 0) = in_fmap_vec.range(
						k * my_N * act_bit + my_N * act_bit - 1,
						k * my_N * act_bit);
				in_fmap_bram[i + k * (ic / ic_vec)][j] = tmp_vec;
			}
		}
	}
}*/

// 324=weight_bit*IC*OC
// all of IC (for 1 output channel) element is contiguous, followed by another IC channel
void weight_S2BRAM(hls::stream<ap_uint<324> >& weight_stream,
		 ap_uint<1> weight0[32][IC / IC_VEC][IC_VEC],
		ap_uint<1> weight1[32][IC / IC_VEC][IC_VEC],
		ap_uint<1> weight2[32][IC / IC_VEC][IC_VEC],
		ap_uint<1> weight3[32][IC / IC_VEC][IC_VEC],
		ap_uint<1> weight4[32][IC / IC_VEC][IC_VEC],
		ap_uint<1> weight5[32][IC / IC_VEC][IC_VEC],
		ap_uint<1> weight6[32][IC / IC_VEC][IC_VEC],
		ap_uint<1> weight7[32][IC / IC_VEC][IC_VEC],
		ap_uint<1> weight8[32][IC / IC_VEC][IC_VEC],
		ap_uint<1> weight9[32][IC / IC_VEC][IC_VEC],
		ap_uint<1> weight10[32][IC / IC_VEC][IC_VEC],
		ap_uint<1> weight11[32][IC / IC_VEC][IC_VEC],
    int total_ic) {

	for(int j=0; j<total_ic; j++){

		ap_uint<324> weight_vec = weight_stream.read();

		for (int i = 0; i < (IC / IC_VEC); i++) {

      weight0[j][i][0] = weight_vec.range(0+IC_VEC*i,0+IC_VEC*i);
      weight0[j][i][1] = weight_vec.range(1+IC_VEC*i,1+IC_VEC*i);
      weight0[j][i][2] = weight_vec.range(2+IC_VEC*i,2+IC_VEC*i);
      weight0[j][i][3] = weight_vec.range(3+IC_VEC*i,3+IC_VEC*i);
      weight0[j][i][4] = weight_vec.range(4+IC_VEC*i,4+IC_VEC*i);
      weight0[j][i][5] = weight_vec.range(5+IC_VEC*i,5+IC_VEC*i);
      weight1[j][i][0] = weight_vec.range(12+IC_VEC*i,12+IC_VEC*i);
      weight1[j][i][1] = weight_vec.range(13+IC_VEC*i,13+IC_VEC*i);
      weight1[j][i][2] = weight_vec.range(14+IC_VEC*i,14+IC_VEC*i);
      weight1[j][i][3] = weight_vec.range(15+IC_VEC*i,15+IC_VEC*i);
      weight1[j][i][4] = weight_vec.range(16+IC_VEC*i,16+IC_VEC*i);
      weight1[j][i][5] = weight_vec.range(17+IC_VEC*i,17+IC_VEC*i);
      weight2[j][i][0] = weight_vec.range(24+IC_VEC*i,24+IC_VEC*i);
      weight2[j][i][1] = weight_vec.range(25+IC_VEC*i,25+IC_VEC*i);
      weight2[j][i][2] = weight_vec.range(26+IC_VEC*i,26+IC_VEC*i);
      weight2[j][i][3] = weight_vec.range(27+IC_VEC*i,27+IC_VEC*i);
      weight2[j][i][4] = weight_vec.range(28+IC_VEC*i,28+IC_VEC*i);
      weight2[j][i][5] = weight_vec.range(29+IC_VEC*i,29+IC_VEC*i);
      weight3[j][i][0] = weight_vec.range(36+IC_VEC*i,36+IC_VEC*i);
      weight3[j][i][1] = weight_vec.range(37+IC_VEC*i,37+IC_VEC*i);
      weight3[j][i][2] = weight_vec.range(38+IC_VEC*i,38+IC_VEC*i);
      weight3[j][i][3] = weight_vec.range(39+IC_VEC*i,39+IC_VEC*i);
      weight3[j][i][4] = weight_vec.range(40+IC_VEC*i,40+IC_VEC*i);
      weight3[j][i][5] = weight_vec.range(41+IC_VEC*i,41+IC_VEC*i);
      weight4[j][i][0] = weight_vec.range(48+IC_VEC*i,48+IC_VEC*i);
      weight4[j][i][1] = weight_vec.range(49+IC_VEC*i,49+IC_VEC*i);
      weight4[j][i][2] = weight_vec.range(50+IC_VEC*i,50+IC_VEC*i);
      weight4[j][i][3] = weight_vec.range(51+IC_VEC*i,51+IC_VEC*i);
      weight4[j][i][4] = weight_vec.range(52+IC_VEC*i,52+IC_VEC*i);
      weight4[j][i][5] = weight_vec.range(53+IC_VEC*i,53+IC_VEC*i);
      weight5[j][i][0] = weight_vec.range(60+IC_VEC*i,60+IC_VEC*i);
      weight5[j][i][1] = weight_vec.range(61+IC_VEC*i,61+IC_VEC*i);
      weight5[j][i][2] = weight_vec.range(62+IC_VEC*i,62+IC_VEC*i);
      weight5[j][i][3] = weight_vec.range(63+IC_VEC*i,63+IC_VEC*i);
      weight5[j][i][4] = weight_vec.range(64+IC_VEC*i,64+IC_VEC*i);
      weight5[j][i][5] = weight_vec.range(65+IC_VEC*i,65+IC_VEC*i);
      weight6[j][i][0] = weight_vec.range(72+IC_VEC*i,72+IC_VEC*i);
      weight6[j][i][1] = weight_vec.range(73+IC_VEC*i,73+IC_VEC*i);
      weight6[j][i][2] = weight_vec.range(74+IC_VEC*i,74+IC_VEC*i);
      weight6[j][i][3] = weight_vec.range(75+IC_VEC*i,75+IC_VEC*i);
      weight6[j][i][4] = weight_vec.range(76+IC_VEC*i,76+IC_VEC*i);
      weight6[j][i][5] = weight_vec.range(77+IC_VEC*i,77+IC_VEC*i);
      weight7[j][i][0] = weight_vec.range(84+IC_VEC*i,84+IC_VEC*i);
      weight7[j][i][1] = weight_vec.range(85+IC_VEC*i,85+IC_VEC*i);
      weight7[j][i][2] = weight_vec.range(86+IC_VEC*i,86+IC_VEC*i);
      weight7[j][i][3] = weight_vec.range(87+IC_VEC*i,87+IC_VEC*i);
      weight7[j][i][4] = weight_vec.range(88+IC_VEC*i,88+IC_VEC*i);
      weight7[j][i][5] = weight_vec.range(89+IC_VEC*i,89+IC_VEC*i);
      weight8[j][i][0] = weight_vec.range(96+IC_VEC*i,96+IC_VEC*i);
      weight8[j][i][1] = weight_vec.range(97+IC_VEC*i,97+IC_VEC*i);
      weight8[j][i][2] = weight_vec.range(98+IC_VEC*i,98+IC_VEC*i);
      weight8[j][i][3] = weight_vec.range(99+IC_VEC*i,99+IC_VEC*i);
      weight8[j][i][4] = weight_vec.range(100+IC_VEC*i,100+IC_VEC*i);
      weight8[j][i][5] = weight_vec.range(101+IC_VEC*i,101+IC_VEC*i);
      weight9[j][i][0] = weight_vec.range(108+IC_VEC*i,108+IC_VEC*i);
      weight9[j][i][1] = weight_vec.range(109+IC_VEC*i,109+IC_VEC*i);
      weight9[j][i][2] = weight_vec.range(110+IC_VEC*i,110+IC_VEC*i);
      weight9[j][i][3] = weight_vec.range(111+IC_VEC*i,111+IC_VEC*i);
      weight9[j][i][4] = weight_vec.range(112+IC_VEC*i,112+IC_VEC*i);
      weight9[j][i][5] = weight_vec.range(113+IC_VEC*i,113+IC_VEC*i);
      weight10[j][i][0] = weight_vec.range(120+IC_VEC*i,120+IC_VEC*i);
      weight10[j][i][1] = weight_vec.range(121+IC_VEC*i,121+IC_VEC*i);
      weight10[j][i][2] = weight_vec.range(122+IC_VEC*i,122+IC_VEC*i);
      weight10[j][i][3] = weight_vec.range(123+IC_VEC*i,123+IC_VEC*i);
      weight10[j][i][4] = weight_vec.range(124+IC_VEC*i,124+IC_VEC*i);
      weight10[j][i][5] = weight_vec.range(125+IC_VEC*i,125+IC_VEC*i);
      weight11[j][i][0] = weight_vec.range(132+IC_VEC*i,132+IC_VEC*i);
      weight11[j][i][1] = weight_vec.range(133+IC_VEC*i,133+IC_VEC*i);
      weight11[j][i][2] = weight_vec.range(134+IC_VEC*i,134+IC_VEC*i);
      weight11[j][i][3] = weight_vec.range(135+IC_VEC*i,135+IC_VEC*i);
      weight11[j][i][4] = weight_vec.range(136+IC_VEC*i,136+IC_VEC*i);
      weight11[j][i][5] = weight_vec.range(137+IC_VEC*i,137+IC_VEC*i);
    }
	}
}

ap_uint<4> convert(ap_uint<13> input, ap_uint<13> threshold[OUT_ACT_NUM]) {

	ap_uint<4> ret = 0;

/* Tree `
	if (input < threshold[7]){
	    if (input < threshold[3]){
	        if (input < threshold[1]){
	            if (input < threshold[0]){
		            ret = (ap_uint<4> ) 0.0;
                } else {  // 1 
		            ret = (ap_uint<4> ) 1.0;
                }
            } else {
	            if (input < threshold[2]){
		            ret = (ap_uint<4> ) 2.0;
                } else { // 3
		            ret = (ap_uint<4> ) 3.0;
                }
            }
        } else {
	        if (input < threshold[5]){
	            if (input < threshold[4]){
		            ret = (ap_uint<4> ) 4.0;
                } else {
		            ret = (ap_uint<4> ) 5.0;
                }
            } else {
	            if (input < threshold[6]){
		            ret = (ap_uint<4> ) 6.0;
                } else {
		            ret = (ap_uint<4> ) 7.0;
                }
            }
        }
    } else {
	    if (input < threshold[11]){
	        if (input < threshold[9]){
	            if (input < threshold[8]){
		            ret = (ap_uint<4> ) 8.0;
                } else {  // 1 
		            ret = (ap_uint<4> ) 9.0;
                }
            } else {
	            if (input < threshold[10]){
		            ret = (ap_uint<4> ) 10.0;
                } else { // 3
		            ret = (ap_uint<4> ) 11.0;
                }
            }
        } else {
	        if (input < threshold[13]){
	            if (input < threshold[12]){
		            ret = (ap_uint<4> ) 12.0;
                } else {
		            ret = (ap_uint<4> ) 13.0;
                }
            } else {
	            if (input < threshold[14]){
		            ret = (ap_uint<4> ) 14.0;
                } else {
		            ret = (ap_uint<4> ) 15.0;
                }
            }
        }
    }
*/
	if (input < threshold[0])
		ret = (ap_uint<4> ) 0;
	else if (input < threshold[1])
		ret = (ap_uint<4> ) 1;
	else if (input < threshold[2])
		ret = (ap_uint<4> ) 2;
	else if (input < threshold[3])
		ret = (ap_uint<4> ) 3;
	else if (input < threshold[4])
		ret = (ap_uint<4> ) 4;
	else if (input < threshold[5])
		ret = (ap_uint<4> ) 5;
	else if (input < threshold[6])
		ret = (ap_uint<4> ) 6;
	else if (input < threshold[7])
		ret = (ap_uint<4> ) 7;
	else if (input < threshold[8])
		ret = (ap_uint<4> ) 8;
	else if (input < threshold[9])
		ret = (ap_uint<4> ) 9;
	else if (input < threshold[10])
		ret = (ap_uint<4> ) 10;
	else if (input < threshold[11])
		ret = (ap_uint<4> ) 11;
	else if (input < threshold[12])
		ret = (ap_uint<4> ) 12;
	else if (input < threshold[13])
		ret = (ap_uint<4> ) 13;
	else if (input < threshold[14])
		ret = (ap_uint<4> ) 14;
	else
		ret = (ap_uint<4> ) 15;
	return ret;
}

void conv(ap_uint<36> in_fmap[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight0[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight1[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight2[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight3[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight4[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight5[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight6[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight7[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight8[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight9[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight10[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight11[IC / IC_VEC][IC_VEC],
		ap_uint<13> partial_sum_local[OC][N]);

// 128=act_bit*WIDTH
void store_out_fmap(ap_uint<36> out_fmap_bram[128/N/ACT_BIT][4][OC], ap_uint<128>* out_fmap, ap_uint<13> threshold[OUT_ACT_NUM], int height, int total_oc, int h_iter, int oc_t_iter, int shuffle_index, bool is_shift){
    total_oc = total_oc*OC;
	int base_addr1=oc_t_iter*OC;
	int base_addr2=h_iter*total_oc;
	int base_shift_index=base_addr1%5;
	//int base_shift_index=base_addr1-(base_addr1/5)*5;
	int shift_index=base_shift_index;
	for(int i=0; i<OC; i++){
#pragma HLS PIPELINE

        ap_uint<128> out_fmap_vec;
        for(int j=0; j<128/ N /ACT_BIT; j++){
            ap_uint<32> converted_vec;

            ap_uint<13> tmp_vec0;
            ap_uint<13> tmp_vec1;
            ap_uint<13> tmp_vec2;
            ap_uint<13> tmp_vec3;
            ap_uint<13> tmp_vec4;
            ap_uint<13> tmp_vec5;
            ap_uint<13> tmp_vec6;
            ap_uint<13> tmp_vec7;

            tmp_vec0.range(3,0)=out_fmap_bram[j][0][i].range(3,0);
            tmp_vec0.range(7,4)=out_fmap_bram[j][1][i].range(3,0);
            tmp_vec0.range(11,8)=out_fmap_bram[j][2][i].range(3,0);
            tmp_vec0.range(12,12)=out_fmap_bram[j][3][i].range(0,0);

            tmp_vec1.range(3,0)=out_fmap_bram[j][0][i].range(7,4);
            tmp_vec1.range(7,4)=out_fmap_bram[j][1][i].range(7,4);
            tmp_vec1.range(11,8)=out_fmap_bram[j][2][i].range(7,4);
            tmp_vec1.range(12,12)=out_fmap_bram[j][3][i].range(4,4);

            tmp_vec2.range(3,0)=out_fmap_bram[j][0][i].range(11,8);
            tmp_vec2.range(7,4)=out_fmap_bram[j][1][i].range(11,8);
            tmp_vec2.range(11,8)=out_fmap_bram[j][2][i].range(11,8);
            tmp_vec2.range(12,12)=out_fmap_bram[j][3][i].range(8,8);

            tmp_vec3.range(3,0)=out_fmap_bram[j][0][i].range(15,12);
            tmp_vec3.range(7,4)=out_fmap_bram[j][1][i].range(15,12);
            tmp_vec3.range(11,8)=out_fmap_bram[j][2][i].range(15,12);
            tmp_vec3.range(12,12)=out_fmap_bram[j][3][i].range(12,12);

            tmp_vec4.range(3,0)=out_fmap_bram[j][0][i].range(19,16);
            tmp_vec4.range(7,4)=out_fmap_bram[j][1][i].range(19,16);
            tmp_vec4.range(11,8)=out_fmap_bram[j][2][i].range(19,16);
            tmp_vec4.range(12,12)=out_fmap_bram[j][3][i].range(16,16);

            tmp_vec5.range(3,0)=out_fmap_bram[j][0][i].range(23,20);
            tmp_vec5.range(7,4)=out_fmap_bram[j][1][i].range(23,20);
            tmp_vec5.range(11,8)=out_fmap_bram[j][2][i].range(23,20);
            tmp_vec5.range(12,12)=out_fmap_bram[j][3][i].range(20,20);

            tmp_vec6.range(3,0)=out_fmap_bram[j][0][i].range(27,24);
            tmp_vec6.range(7,4)=out_fmap_bram[j][1][i].range(27,24);
            tmp_vec6.range(11,8)=out_fmap_bram[j][2][i].range(27,24);
            tmp_vec6.range(12,12)=out_fmap_bram[j][3][i].range(24,24);

            tmp_vec7.range(3,0)=out_fmap_bram[j][0][i].range(31,28);
            tmp_vec7.range(7,4)=out_fmap_bram[j][1][i].range(31,28);
            tmp_vec7.range(11,8)=out_fmap_bram[j][2][i].range(31,28);
            tmp_vec7.range(12,12)=out_fmap_bram[j][3][i].range(28,28);

            converted_vec.range(3,0)=convert(tmp_vec0, threshold);
            converted_vec.range(7,4)=convert(tmp_vec1, threshold);
            converted_vec.range(11,8)=convert(tmp_vec2, threshold);
            converted_vec.range(15,12)=convert(tmp_vec3, threshold);
            converted_vec.range(19,16)=convert(tmp_vec4, threshold);
            converted_vec.range(23,20)=convert(tmp_vec5, threshold);
            converted_vec.range(27,24)=convert(tmp_vec6, threshold);
            converted_vec.range(31,28)=convert(tmp_vec7, threshold);


            out_fmap_vec.range(j* N*ACT_BIT+ N*ACT_BIT-1, j* N*ACT_BIT)=converted_vec;
        }
        //out_fmap_stream.write(out_fmap_vec);
        int channel_index=i+base_addr1;
        int out_fmap_addr=channel_index+base_addr2;
        if(is_shift){
                
				switch (shift_index)
				{
					case 0: {
						//shift down
                    	//consider index overflow
						if(h_iter==(height-1)){
							out_fmap_addr=out_fmap_addr-(height-1)*total_oc;
							out_fmap_vec=0;
						} else {
							out_fmap_addr=out_fmap_addr+total_oc;
						}
						break;
					}
					case 1: {
						//shift right
						ap_uint<128> tmp_vec;
						tmp_vec.range(3,0)=0;
						tmp_vec.range(127,4)=out_fmap_vec.range(123,0);
						out_fmap_vec=tmp_vec;
						break;
					}
					case 3: {
						//shift left
						ap_uint<128> tmp_vec;
						tmp_vec.range(127,124)=0;
						tmp_vec.range(123,0)=out_fmap_vec.range(127,4);
						out_fmap_vec=tmp_vec;
						break;
					}
					case 4: {
						//shift up
						//consider index overflow
						if(h_iter==0){
							out_fmap_addr=out_fmap_addr+(height-1)*total_oc;
							out_fmap_vec=0;
						} else
							out_fmap_addr=out_fmap_addr-total_oc;
						break;
					}
				
					default:
						break;
				}
				
				
				/*if(shift_index==0){
                    //shift down
                    //consider index overflow
                    if(h_iter==(height-1)){
						out_fmap_addr=out_fmap_addr-(height-1)*total_oc;
						out_fmap_vec=0;
					} else {
						out_fmap_addr=out_fmap_addr+total_oc;
					}
                }
                else if(shift_index==1){
                    //shift right
                    ap_uint<128> tmp_vec;
                    tmp_vec.range(3,0)=0;
                    tmp_vec.range(127,4)=out_fmap_vec.range(123,0);
					out_fmap_vec=tmp_vec;
                }
                else if(shift_index==2){
                    //identity
                    ;
                }
                else if(shift_index==3){
                    //shift left
                    ap_uint<128> tmp_vec;
                    tmp_vec.range(127,124)=0;
                    tmp_vec.range(123,0)=out_fmap_vec.range(127,4);
					out_fmap_vec=tmp_vec;
                }
                else{
                    //shift up
                    //consider index overflow
                    if(h_iter==0){
						out_fmap_addr=out_fmap_addr+(height-1)*total_oc;
						out_fmap_vec=0;
					} else
						out_fmap_addr=out_fmap_addr-total_oc;
                }*/
            }
			out_fmap[out_fmap_addr]=out_fmap_vec;
			shift_index++;
			if(shift_index==5)
				shift_index=0;
    }
}


// store 2 bram together
void store_out_fmap2(ap_uint<36> out_fmap_bram0[128/N/ACT_BIT][4][OC], ap_uint<36> out_fmap_bram1[128/N/ACT_BIT][4][OC], ap_uint<128>* out_fmap, ap_uint<13> threshold[OUT_ACT_NUM], int height, int total_oc, int h_iter, int oc_t_iter, int shuffle_index, bool is_shift){
	total_oc = total_oc*OC;
	int base_addr1=oc_t_iter*OC;
	int base_addr2=h_iter*total_oc;
	int base_shift_index=base_addr1%5;
	//int base_shift_index=base_addr1-(base_addr1/5)*5;
	int shift_index=base_shift_index;

	for(int i=0; i<OC; i++){
#pragma HLS PIPELINE

        ap_uint<128> out_fmap_vec;
        for(int j=0; j<128/ N /ACT_BIT; j++){
            ap_uint<32> converted_vec;

            ap_uint<13> tmp_vec0;
            ap_uint<13> tmp_vec1;
            ap_uint<13> tmp_vec2;
            ap_uint<13> tmp_vec3;
            ap_uint<13> tmp_vec4;
            ap_uint<13> tmp_vec5;
            ap_uint<13> tmp_vec6;
            ap_uint<13> tmp_vec7;

            tmp_vec0.range(3,0)=out_fmap_bram0[j][0][i].range(3,0);
            tmp_vec0.range(7,4)=out_fmap_bram0[j][1][i].range(3,0);
            tmp_vec0.range(11,8)=out_fmap_bram0[j][2][i].range(3,0);
            tmp_vec0.range(12,12)=out_fmap_bram0[j][3][i].range(0,0);

            tmp_vec1.range(3,0)=out_fmap_bram0[j][0][i].range(7,4);
            tmp_vec1.range(7,4)=out_fmap_bram0[j][1][i].range(7,4);
            tmp_vec1.range(11,8)=out_fmap_bram0[j][2][i].range(7,4);
            tmp_vec1.range(12,12)=out_fmap_bram0[j][3][i].range(4,4);

            tmp_vec2.range(3,0)=out_fmap_bram0[j][0][i].range(11,8);
            tmp_vec2.range(7,4)=out_fmap_bram0[j][1][i].range(11,8);
            tmp_vec2.range(11,8)=out_fmap_bram0[j][2][i].range(11,8);
            tmp_vec2.range(12,12)=out_fmap_bram0[j][3][i].range(8,8);

            tmp_vec3.range(3,0)=out_fmap_bram0[j][0][i].range(15,12);
            tmp_vec3.range(7,4)=out_fmap_bram0[j][1][i].range(15,12);
            tmp_vec3.range(11,8)=out_fmap_bram0[j][2][i].range(15,12);
            tmp_vec3.range(12,12)=out_fmap_bram0[j][3][i].range(12,12);

            tmp_vec4.range(3,0)=out_fmap_bram0[j][0][i].range(19,16);
            tmp_vec4.range(7,4)=out_fmap_bram0[j][1][i].range(19,16);
            tmp_vec4.range(11,8)=out_fmap_bram0[j][2][i].range(19,16);
            tmp_vec4.range(12,12)=out_fmap_bram0[j][3][i].range(16,16);

            tmp_vec5.range(3,0)=out_fmap_bram0[j][0][i].range(23,20);
            tmp_vec5.range(7,4)=out_fmap_bram0[j][1][i].range(23,20);
            tmp_vec5.range(11,8)=out_fmap_bram0[j][2][i].range(23,20);
            tmp_vec5.range(12,12)=out_fmap_bram0[j][3][i].range(20,20);

            tmp_vec6.range(3,0)=out_fmap_bram0[j][0][i].range(27,24);
            tmp_vec6.range(7,4)=out_fmap_bram0[j][1][i].range(27,24);
            tmp_vec6.range(11,8)=out_fmap_bram0[j][2][i].range(27,24);
            tmp_vec6.range(12,12)=out_fmap_bram0[j][3][i].range(24,24);

            tmp_vec7.range(3,0)=out_fmap_bram0[j][0][i].range(31,28);
            tmp_vec7.range(7,4)=out_fmap_bram0[j][1][i].range(31,28);
            tmp_vec7.range(11,8)=out_fmap_bram0[j][2][i].range(31,28);
            tmp_vec7.range(12,12)=out_fmap_bram0[j][3][i].range(28,28);

            converted_vec.range(3,0)=convert(tmp_vec0, threshold);
            converted_vec.range(7,4)=convert(tmp_vec1, threshold);
            converted_vec.range(11,8)=convert(tmp_vec2, threshold);
            converted_vec.range(15,12)=convert(tmp_vec3, threshold);
            converted_vec.range(19,16)=convert(tmp_vec4, threshold);
            converted_vec.range(23,20)=convert(tmp_vec5, threshold);
            converted_vec.range(27,24)=convert(tmp_vec6, threshold);
            converted_vec.range(31,28)=convert(tmp_vec7, threshold);


            out_fmap_vec.range(j* N*ACT_BIT+ N*ACT_BIT-1, j* N*ACT_BIT)=converted_vec;
        }
        //out_fmap_stream.write(out_fmap_vec);
        int channel_index=i+base_addr1;
        int out_fmap_addr=channel_index+base_addr2;
        if(is_shift){
                
				switch (shift_index)
				{
					case 0: {
						//shift down
                    	//consider index overflow
						if(h_iter==(height-1)){
							out_fmap_addr=out_fmap_addr-(height-1)*total_oc;
							out_fmap_vec=0;
						} else {
							out_fmap_addr=out_fmap_addr+total_oc;
						}
						break;
					}
					case 1: {
						//shift right
						ap_uint<128> tmp_vec;
						tmp_vec.range(3,0)=0;
						tmp_vec.range(127,4)=out_fmap_vec.range(123,0);
						out_fmap_vec=tmp_vec;
						break;
					}
					case 3: {
						//shift left
						ap_uint<128> tmp_vec;
						tmp_vec.range(127,124)=0;
						tmp_vec.range(123,0)=out_fmap_vec.range(127,4);
						out_fmap_vec=tmp_vec;
						break;
					}
					case 4: {
						//shift up
						//consider index overflow
						if(h_iter==0){
							out_fmap_addr=out_fmap_addr+(height-1)*total_oc;
							out_fmap_vec=0;
						} else
							out_fmap_addr=out_fmap_addr-total_oc;
						break;
					}
				
					default:
						break;
				}
            }
			out_fmap[out_fmap_addr]=out_fmap_vec;
			shift_index++;
			if(shift_index==5)
				shift_index=0;
    }

	h_iter++;

	base_addr2=h_iter*total_oc;
	shift_index=base_shift_index;

	for(int i=0; i<OC; i++){
#pragma HLS PIPELINE

        ap_uint<128> out_fmap_vec;
        for(int j=0; j<128/ N /ACT_BIT; j++){
            ap_uint<32> converted_vec;

            ap_uint<13> tmp_vec0;
            ap_uint<13> tmp_vec1;
            ap_uint<13> tmp_vec2;
            ap_uint<13> tmp_vec3;
            ap_uint<13> tmp_vec4;
            ap_uint<13> tmp_vec5;
            ap_uint<13> tmp_vec6;
            ap_uint<13> tmp_vec7;

            tmp_vec0.range(3,0)=out_fmap_bram1[j][0][i].range(3,0);
            tmp_vec0.range(7,4)=out_fmap_bram1[j][1][i].range(3,0);
            tmp_vec0.range(11,8)=out_fmap_bram1[j][2][i].range(3,0);
            tmp_vec0.range(12,12)=out_fmap_bram1[j][3][i].range(0,0);

            tmp_vec1.range(3,0)=out_fmap_bram1[j][0][i].range(7,4);
            tmp_vec1.range(7,4)=out_fmap_bram1[j][1][i].range(7,4);
            tmp_vec1.range(11,8)=out_fmap_bram1[j][2][i].range(7,4);
            tmp_vec1.range(12,12)=out_fmap_bram1[j][3][i].range(4,4);

            tmp_vec2.range(3,0)=out_fmap_bram1[j][0][i].range(11,8);
            tmp_vec2.range(7,4)=out_fmap_bram1[j][1][i].range(11,8);
            tmp_vec2.range(11,8)=out_fmap_bram1[j][2][i].range(11,8);
            tmp_vec2.range(12,12)=out_fmap_bram1[j][3][i].range(8,8);

            tmp_vec3.range(3,0)=out_fmap_bram1[j][0][i].range(15,12);
            tmp_vec3.range(7,4)=out_fmap_bram1[j][1][i].range(15,12);
            tmp_vec3.range(11,8)=out_fmap_bram1[j][2][i].range(15,12);
            tmp_vec3.range(12,12)=out_fmap_bram1[j][3][i].range(12,12);

            tmp_vec4.range(3,0)=out_fmap_bram1[j][0][i].range(19,16);
            tmp_vec4.range(7,4)=out_fmap_bram1[j][1][i].range(19,16);
            tmp_vec4.range(11,8)=out_fmap_bram1[j][2][i].range(19,16);
            tmp_vec4.range(12,12)=out_fmap_bram1[j][3][i].range(16,16);

            tmp_vec5.range(3,0)=out_fmap_bram1[j][0][i].range(23,20);
            tmp_vec5.range(7,4)=out_fmap_bram1[j][1][i].range(23,20);
            tmp_vec5.range(11,8)=out_fmap_bram1[j][2][i].range(23,20);
            tmp_vec5.range(12,12)=out_fmap_bram1[j][3][i].range(20,20);

            tmp_vec6.range(3,0)=out_fmap_bram1[j][0][i].range(27,24);
            tmp_vec6.range(7,4)=out_fmap_bram1[j][1][i].range(27,24);
            tmp_vec6.range(11,8)=out_fmap_bram1[j][2][i].range(27,24);
            tmp_vec6.range(12,12)=out_fmap_bram1[j][3][i].range(24,24);

            tmp_vec7.range(3,0)=out_fmap_bram1[j][0][i].range(31,28);
            tmp_vec7.range(7,4)=out_fmap_bram1[j][1][i].range(31,28);
            tmp_vec7.range(11,8)=out_fmap_bram1[j][2][i].range(31,28);
            tmp_vec7.range(12,12)=out_fmap_bram1[j][3][i].range(28,28);

            converted_vec.range(3,0)=convert(tmp_vec0, threshold);
            converted_vec.range(7,4)=convert(tmp_vec1, threshold);
            converted_vec.range(11,8)=convert(tmp_vec2, threshold);
            converted_vec.range(15,12)=convert(tmp_vec3, threshold);
            converted_vec.range(19,16)=convert(tmp_vec4, threshold);
            converted_vec.range(23,20)=convert(tmp_vec5, threshold);
            converted_vec.range(27,24)=convert(tmp_vec6, threshold);
            converted_vec.range(31,28)=convert(tmp_vec7, threshold);


            out_fmap_vec.range(j* N*ACT_BIT+ N*ACT_BIT-1, j* N*ACT_BIT)=converted_vec;
        }
        //out_fmap_stream.write(out_fmap_vec);
        int channel_index=i+base_addr1;
        int out_fmap_addr=channel_index+base_addr2;
        if(is_shift){
                
				switch (shift_index)
				{
					case 0: {
						//shift down
                    	//consider index overflow
						if(h_iter==(height-1)){
							out_fmap_addr=out_fmap_addr-(height-1)*total_oc;
							out_fmap_vec=0;
						} else {
							out_fmap_addr=out_fmap_addr+total_oc;
						}
						break;
					}
					case 1: {
						//shift right
						ap_uint<128> tmp_vec;
						tmp_vec.range(3,0)=0;
						tmp_vec.range(127,4)=out_fmap_vec.range(123,0);
						out_fmap_vec=tmp_vec;
						break;
					}
					case 3: {
						//shift left
						ap_uint<128> tmp_vec;
						tmp_vec.range(127,124)=0;
						tmp_vec.range(123,0)=out_fmap_vec.range(127,4);
						out_fmap_vec=tmp_vec;
						break;
					}
					case 4: {
						//shift up
						//consider index overflow
						if(h_iter==0){
							out_fmap_addr=out_fmap_addr+(height-1)*total_oc;
							out_fmap_vec=0;
						} else
							out_fmap_addr=out_fmap_addr-total_oc;
						break;
					}
				
					default:
						break;
				}
            }
			out_fmap[out_fmap_addr]=out_fmap_vec;
			shift_index++;
			if(shift_index==5)
				shift_index=0;
    }

}

ap_uint<4> my_max(ap_uint<4> a, ap_uint<4> b){
	if(a>b)
		return a;
	return b;
}

ap_uint<128> max_pool(ap_uint<128> a, ap_uint<128> b) {
	ap_uint<128> result;
	result.range(127,64)=0;
	for(int i=0; i<16; i++){
#pragma HLS UNROLL

		ap_uint<4> tmp1=a.range(i*8+3,i*8);
		ap_uint<4> tmp2=a.range(i*8+7,i*8+4);
		ap_uint<4> tmp3=b.range(i*8+3,i*8);
		ap_uint<4> tmp4=b.range(i*8+7,i*8+4);
		result.range(i*4+3,i*4)=my_max(my_max(tmp1, tmp2), my_max(tmp3, tmp4));
	}
	return result;
}


// store 2 bram together
// h_iter is input h iter
// height is output height
void store_out_fmap3(ap_uint<36> out_fmap_bram0[128/N/ACT_BIT][4][OC], ap_uint<36> out_fmap_bram1[128/N/ACT_BIT][4][OC], ap_uint<128>* out_fmap, ap_uint<13> threshold[OUT_ACT_NUM], int height, int total_oc, int h_iter, int oc_t_iter, int shuffle_index, bool is_shift, bool is_pooling){
	if(is_pooling)
		height=height/2;
	total_oc = total_oc*OC;
	int base_addr1=oc_t_iter*OC;
	int base_addr2=h_iter*total_oc;
	int base_addr3=h_iter*total_oc/2;//for pooling
	//int base_shift_index=base_addr1%5;
	ap_uint<3> base_shift_index=base_addr1 & 3;
	//int base_shift_index=base_addr1-(base_addr1/5)*5;
	ap_uint<3> shift_index=base_shift_index;

	for(int i=0; i<OC; i++){
#pragma HLS PIPELINE

        ap_uint<128> out_fmap_vec0;
		ap_uint<128> out_fmap_vec1;
        for(int j=0; j<128/ N /ACT_BIT; j++){
            ap_uint<32> converted_vec0;
			ap_uint<32> converted_vec1;

            ap_uint<13> tmp_vec00;
            ap_uint<13> tmp_vec01;
            ap_uint<13> tmp_vec02;
            ap_uint<13> tmp_vec03;
            ap_uint<13> tmp_vec04;
            ap_uint<13> tmp_vec05;
            ap_uint<13> tmp_vec06;
            ap_uint<13> tmp_vec07;
			
			ap_uint<13> tmp_vec10;
            ap_uint<13> tmp_vec11;
            ap_uint<13> tmp_vec12;
            ap_uint<13> tmp_vec13;
            ap_uint<13> tmp_vec14;
            ap_uint<13> tmp_vec15;
            ap_uint<13> tmp_vec16;
            ap_uint<13> tmp_vec17;

            tmp_vec00.range(3,0)=out_fmap_bram0[j][0][i].range(3,0);
            tmp_vec00.range(7,4)=out_fmap_bram0[j][1][i].range(3,0);
            tmp_vec00.range(11,8)=out_fmap_bram0[j][2][i].range(3,0);
            tmp_vec00.range(12,12)=out_fmap_bram0[j][3][i].range(0,0);

            tmp_vec01.range(3,0)=out_fmap_bram0[j][0][i].range(7,4);
            tmp_vec01.range(7,4)=out_fmap_bram0[j][1][i].range(7,4);
            tmp_vec01.range(11,8)=out_fmap_bram0[j][2][i].range(7,4);
            tmp_vec01.range(12,12)=out_fmap_bram0[j][3][i].range(4,4);

            tmp_vec02.range(3,0)=out_fmap_bram0[j][0][i].range(11,8);
            tmp_vec02.range(7,4)=out_fmap_bram0[j][1][i].range(11,8);
            tmp_vec02.range(11,8)=out_fmap_bram0[j][2][i].range(11,8);
            tmp_vec02.range(12,12)=out_fmap_bram0[j][3][i].range(8,8);

            tmp_vec03.range(3,0)=out_fmap_bram0[j][0][i].range(15,12);
            tmp_vec03.range(7,4)=out_fmap_bram0[j][1][i].range(15,12);
            tmp_vec03.range(11,8)=out_fmap_bram0[j][2][i].range(15,12);
            tmp_vec03.range(12,12)=out_fmap_bram0[j][3][i].range(12,12);

            tmp_vec04.range(3,0)=out_fmap_bram0[j][0][i].range(19,16);
            tmp_vec04.range(7,4)=out_fmap_bram0[j][1][i].range(19,16);
            tmp_vec04.range(11,8)=out_fmap_bram0[j][2][i].range(19,16);
            tmp_vec04.range(12,12)=out_fmap_bram0[j][3][i].range(16,16);

            tmp_vec05.range(3,0)=out_fmap_bram0[j][0][i].range(23,20);
            tmp_vec05.range(7,4)=out_fmap_bram0[j][1][i].range(23,20);
            tmp_vec05.range(11,8)=out_fmap_bram0[j][2][i].range(23,20);
            tmp_vec05.range(12,12)=out_fmap_bram0[j][3][i].range(20,20);

            tmp_vec06.range(3,0)=out_fmap_bram0[j][0][i].range(27,24);
            tmp_vec06.range(7,4)=out_fmap_bram0[j][1][i].range(27,24);
            tmp_vec06.range(11,8)=out_fmap_bram0[j][2][i].range(27,24);
            tmp_vec06.range(12,12)=out_fmap_bram0[j][3][i].range(24,24);

            tmp_vec07.range(3,0)=out_fmap_bram0[j][0][i].range(31,28);
            tmp_vec07.range(7,4)=out_fmap_bram0[j][1][i].range(31,28);
            tmp_vec07.range(11,8)=out_fmap_bram0[j][2][i].range(31,28);
            tmp_vec07.range(12,12)=out_fmap_bram0[j][3][i].range(28,28);


			tmp_vec10.range(3,0)=out_fmap_bram1[j][0][i].range(3,0);
            tmp_vec10.range(7,4)=out_fmap_bram1[j][1][i].range(3,0);
            tmp_vec10.range(11,8)=out_fmap_bram1[j][2][i].range(3,0);
            tmp_vec10.range(12,12)=out_fmap_bram1[j][3][i].range(0,0);

            tmp_vec11.range(3,0)=out_fmap_bram1[j][0][i].range(7,4);
            tmp_vec11.range(7,4)=out_fmap_bram1[j][1][i].range(7,4);
            tmp_vec11.range(11,8)=out_fmap_bram1[j][2][i].range(7,4);
            tmp_vec11.range(12,12)=out_fmap_bram1[j][3][i].range(4,4);

            tmp_vec12.range(3,0)=out_fmap_bram1[j][0][i].range(11,8);
            tmp_vec12.range(7,4)=out_fmap_bram1[j][1][i].range(11,8);
            tmp_vec12.range(11,8)=out_fmap_bram1[j][2][i].range(11,8);
            tmp_vec12.range(12,12)=out_fmap_bram1[j][3][i].range(8,8);

            tmp_vec13.range(3,0)=out_fmap_bram1[j][0][i].range(15,12);
            tmp_vec13.range(7,4)=out_fmap_bram1[j][1][i].range(15,12);
            tmp_vec13.range(11,8)=out_fmap_bram1[j][2][i].range(15,12);
            tmp_vec13.range(12,12)=out_fmap_bram1[j][3][i].range(12,12);

            tmp_vec14.range(3,0)=out_fmap_bram1[j][0][i].range(19,16);
            tmp_vec14.range(7,4)=out_fmap_bram1[j][1][i].range(19,16);
            tmp_vec14.range(11,8)=out_fmap_bram1[j][2][i].range(19,16);
            tmp_vec14.range(12,12)=out_fmap_bram1[j][3][i].range(16,16);

            tmp_vec15.range(3,0)=out_fmap_bram1[j][0][i].range(23,20);
            tmp_vec15.range(7,4)=out_fmap_bram1[j][1][i].range(23,20);
            tmp_vec15.range(11,8)=out_fmap_bram1[j][2][i].range(23,20);
            tmp_vec15.range(12,12)=out_fmap_bram1[j][3][i].range(20,20);

            tmp_vec16.range(3,0)=out_fmap_bram1[j][0][i].range(27,24);
            tmp_vec16.range(7,4)=out_fmap_bram1[j][1][i].range(27,24);
            tmp_vec16.range(11,8)=out_fmap_bram1[j][2][i].range(27,24);
            tmp_vec16.range(12,12)=out_fmap_bram1[j][3][i].range(24,24);

            tmp_vec17.range(3,0)=out_fmap_bram1[j][0][i].range(31,28);
            tmp_vec17.range(7,4)=out_fmap_bram1[j][1][i].range(31,28);
            tmp_vec17.range(11,8)=out_fmap_bram1[j][2][i].range(31,28);
            tmp_vec17.range(12,12)=out_fmap_bram1[j][3][i].range(28,28);



            converted_vec0.range(3,0)=convert(tmp_vec00, threshold);
            converted_vec0.range(7,4)=convert(tmp_vec01, threshold);
            converted_vec0.range(11,8)=convert(tmp_vec02, threshold);
            converted_vec0.range(15,12)=convert(tmp_vec03, threshold);
            converted_vec0.range(19,16)=convert(tmp_vec04, threshold);
            converted_vec0.range(23,20)=convert(tmp_vec05, threshold);
            converted_vec0.range(27,24)=convert(tmp_vec06, threshold);
            converted_vec0.range(31,28)=convert(tmp_vec07, threshold);

			converted_vec1.range(3,0)=convert(tmp_vec10, threshold);
            converted_vec1.range(7,4)=convert(tmp_vec11, threshold);
            converted_vec1.range(11,8)=convert(tmp_vec12, threshold);
            converted_vec1.range(15,12)=convert(tmp_vec13, threshold);
            converted_vec1.range(19,16)=convert(tmp_vec14, threshold);
            converted_vec1.range(23,20)=convert(tmp_vec15, threshold);
            converted_vec1.range(27,24)=convert(tmp_vec16, threshold);
            converted_vec1.range(31,28)=convert(tmp_vec17, threshold);


            out_fmap_vec0.range(j* N*ACT_BIT+ N*ACT_BIT-1, j* N*ACT_BIT)=converted_vec0;
			out_fmap_vec1.range(j* N*ACT_BIT+ N*ACT_BIT-1, j* N*ACT_BIT)=converted_vec1;
        }
        //out_fmap_stream.write(out_fmap_vec);

		ap_uint<128> out_fmap_vec2=max_pool(out_fmap_vec0,out_fmap_vec1);

        int channel_index=i+base_addr1;
        int out_fmap_addr0=channel_index+base_addr2;
		int out_fmap_addr1=out_fmap_addr0+total_oc;
		int out_fmap_addr2=channel_index+base_addr3;// for pooling addr
        if(is_shift){
                
				switch (shift_index)
				{
					case 0: {
						//shift down
                    	//consider index overflow
						out_fmap_addr0=out_fmap_addr0+total_oc;
						if(h_iter==(height-2)){
							out_fmap_addr1=out_fmap_addr1-(height-1)*total_oc;
							out_fmap_vec1=0;
						} else {
							out_fmap_addr1=out_fmap_addr1+total_oc;
						}
						if((h_iter/2)==(height-1)){
							out_fmap_addr2=out_fmap_addr2-(height-1)*total_oc;
							out_fmap_vec2=0;
						} else {
							out_fmap_addr2=out_fmap_addr2+total_oc;
						}
						break;
					}
					case 1: {
						//shift right
						ap_uint<128> tmp_vec;
						tmp_vec.range(3,0)=0;
						tmp_vec.range(127,4)=out_fmap_vec0.range(123,0);
						out_fmap_vec0=tmp_vec;
						tmp_vec.range(127,4)=out_fmap_vec1.range(123,0);
						out_fmap_vec1=tmp_vec;
						tmp_vec.range(127,4)=out_fmap_vec2.range(123,0);
						out_fmap_vec2=tmp_vec;
						break;
					}
					case 3: {
						//shift left
						ap_uint<128> tmp_vec;
						tmp_vec.range(127,124)=0;
						tmp_vec.range(123,0)=out_fmap_vec0.range(127,4);
						out_fmap_vec0=tmp_vec;
						tmp_vec.range(123,0)=out_fmap_vec1.range(127,4);
						out_fmap_vec1=tmp_vec;
						tmp_vec.range(123,0)=out_fmap_vec2.range(127,4);
						out_fmap_vec2=tmp_vec;
						break;
					}
					case 4: {
						//shift up
						//consider index overflow
						out_fmap_addr1=out_fmap_addr1-total_oc;
						if(h_iter==0){
							out_fmap_addr0=out_fmap_addr0+(height-1)*total_oc;
							out_fmap_vec0=0;
							out_fmap_addr2=out_fmap_addr2+(height-1)*total_oc;
							out_fmap_vec2=0;
						} else {
							out_fmap_addr0=out_fmap_addr0-total_oc;
							out_fmap_addr2=out_fmap_addr2-total_oc;
						}
						break;
					}
				
					default:
						break;
				}
            }

			if(!is_pooling) {
				out_fmap[out_fmap_addr0]=out_fmap_vec0;
				out_fmap[out_fmap_addr1]=out_fmap_vec1;
			} else {
				out_fmap[out_fmap_addr2]=out_fmap_vec2;
			}
			shift_index++;
			if(shift_index==5)
				shift_index=0;
    }
}

//#define in_fmap_w (32 + N - 1) / N
//#define in_fmap_h 32
//#define ic_size 18 / IC
//#define  oc_size 18 / OC

//partial_sum_iter = int(round(float(OC * N * 4) / 256))
//partial_sum_width = min( myround(OC * N * 4, 8), 256)
//void compute(hls::stream<ap_uint<128> >& in_fmap_stream, hls::stream<ap_uint<324> >& weight_stream, hls::stream<ap_uint<128> >& out_fmap_stream, ap_uint<13> threshold_local[OUT_ACT_NUM]);
//void compute(ap_uint<128>* in_fmap, hls::stream<ap_uint<324> >& weight_stream, ap_uint<128>* out_fmap, ap_uint<13> threshold_local[OUT_ACT_NUM]);
void compute(ap_uint<128>* in_fmap, hls::stream<ap_uint<324> >& weight_stream, ap_uint<128>* out_fmap, ap_uint<256>* threshold, int in_fmap_w, int in_fmap_h, int ic_size, int oc_size, bool is_shift, bool is_pool);
//void top(ap_uint<256>* threshold, ap_uint<128>* in_fmap, ap_uint<512>* weight, //multiplicate of 8
void top(ap_uint<256>* threshold, ap_uint<128>* in_fmap, ap_uint<256>* weight1, ap_uint<256>* weight2, //multiplicate of 8
		ap_uint<128>* out_fmap,
	    unsigned int in_fmap_w,
	    unsigned int in_fmap_h,
	    unsigned int ic_size,
	    unsigned int  oc_size,
		unsigned int is_shift,
		unsigned int is_pool
) {
#pragma HLS INTERFACE s_axilite port=return bundle=control
#pragma HLS INTERFACE s_axilite port=threshold bundle=control
#pragma HLS INTERFACE m_axi port=threshold offset=slave bundle=gmem0
#pragma HLS INTERFACE s_axilite port=in_fmap bundle=control
#pragma HLS INTERFACE m_axi port=in_fmap offset=slave depth=1200 bundle=gmem1
#pragma HLS INTERFACE s_axilite port=weight1 bundle=control
#pragma HLS INTERFACE m_axi port=weight1 offset=slave depth=4 bundle=gmem2
#pragma HLS INTERFACE s_axilite port=weight2 bundle=control
#pragma HLS INTERFACE m_axi port=weight2 offset=slave depth=4 bundle=gmem2

#pragma HLS INTERFACE s_axilite port=out_fmap bundle=control
#pragma HLS INTERFACE m_axi port=out_fmap offset=slave depth=1200 bundle=gmem3

#pragma HLS INTERFACE s_axilite port=in_fmap_w bundle=control
#pragma HLS INTERFACE s_axilite port=in_fmap_h bundle=control
#pragma HLS INTERFACE s_axilite port=ic_size bundle=control
#pragma HLS INTERFACE s_axilite port=oc_size bundle=control
#pragma HLS INTERFACE s_axilite port=is_shift bundle=control
#pragma HLS INTERFACE s_axilite port=is_pool bundle=control


#pragma HLS dataflow

//	hls::stream<ap_uint<128> > in_fmap_stream;
//#pragma HLS stream variable=in_fmap_stream depth=8
	hls::stream<ap_uint<324> > weight_stream;
#pragma HLS stream variable=weight_stream depth=64
//	hls::stream<ap_uint<128> > out_fmap_stream;
//#pragma HLS stream variable=out_fmap_stream depth=8

// actually is ap_int


	//fetch_in_fmap(in_fmap, in_fmap_stream, HEIGHT, TOTAL_OC, TOTAL_IC);

	// streaming hides the fetching latency
	fetch_weight(weight1, weight2, weight_stream, ic_size, oc_size, in_fmap_h);

	// corner case
	//in_fmap_S2BRAM(in_fmap_stream, in_fmap_local0, N, ACT_BIT, IC, IC_VEC);
	compute(in_fmap, weight_stream, out_fmap, threshold, in_fmap_w, in_fmap_h, ic_size, oc_size, is_shift, is_pool);

	//need to exclude first iteration and add last iteration
	//store_out_fmap(out_fmap, out_fmap_stream, TOTAL_OC, HEIGHT);

}

void compute(ap_uint<128>* in_fmap, hls::stream<ap_uint<324> >& weight_stream, ap_uint<128>* out_fmap, ap_uint<256>* threshold, int in_fmap_w, int in_fmap_h, int ic_size, int oc_size, bool is_shift, bool is_pool){
//#pragma HLS DATAFLOW
	
	ap_uint<36> in_fmap_local0[128 * IC / N / ACT_BIT / IC_VEC][IC_VEC];
#pragma HLS ARRAY_PARTITION variable=in_fmap_local0 complete dim=2
#pragma HLS ARRAY_PARTITION variable=in_fmap_local0 cyclic factor=4 dim=1

	ap_uint<36> in_fmap_local1[128 * IC / N / ACT_BIT / IC_VEC][IC_VEC];
#pragma HLS ARRAY_PARTITION variable=in_fmap_local1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=in_fmap_local1 cyclic factor=4 dim=1

// actually is ap_int
	ap_uint<13> partial_sum_local[OC][N];
#pragma HLS ARRAY_PARTITION variable=partial_sum_local complete dim=0

	ap_uint<36> partial_sum_bram0[128 / N / ACT_BIT][4][OC];
#pragma HLS ARRAY_PARTITION variable=partial_sum_bram0 complete dim=2
#pragma HLS ARRAY_PARTITION variable=partial_sum_bram0 complete dim=3
	ap_uint<36> partial_sum_bram1[128 / N / ACT_BIT][4][OC];
#pragma HLS ARRAY_PARTITION variable=partial_sum_bram1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=partial_sum_bram1 complete dim=3
	ap_uint<36> partial_sum_bram2[128 / N / ACT_BIT][4][OC];
#pragma HLS ARRAY_PARTITION variable=partial_sum_bram2 complete dim=2
#pragma HLS ARRAY_PARTITION variable=partial_sum_bram2 complete dim=3
	ap_uint<36> partial_sum_bram3[128 / N / ACT_BIT][4][OC];
#pragma HLS ARRAY_PARTITION variable=partial_sum_bram3 complete dim=2
#pragma HLS ARRAY_PARTITION variable=partial_sum_bram3 complete dim=3

	// prepare for the weight
	ap_uint<1> weight0[32][IC / IC_VEC][IC_VEC];
#pragma HLS ARRAY_PARTITION variable=weight0 complete dim=2
#pragma HLS ARRAY_PARTITION variable=weight0 complete dim=3

    ap_uint<1> weight1[32][IC / IC_VEC][IC_VEC];
#pragma HLS ARRAY_PARTITION variable=weight1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=weight1 complete dim=3

    ap_uint<1> weight2[32][IC / IC_VEC][IC_VEC];
#pragma HLS ARRAY_PARTITION variable=weight2 complete dim=2
#pragma HLS ARRAY_PARTITION variable=weight2 complete dim=3

    ap_uint<1> weight3[32][IC / IC_VEC][IC_VEC];
#pragma HLS ARRAY_PARTITION variable=weight3 complete dim=2
#pragma HLS ARRAY_PARTITION variable=weight3 complete dim=3

    ap_uint<1> weight4[32][IC / IC_VEC][IC_VEC];
#pragma HLS ARRAY_PARTITION variable=weight4 complete dim=2
#pragma HLS ARRAY_PARTITION variable=weight4 complete dim=3

    ap_uint<1> weight5[32][IC / IC_VEC][IC_VEC];
#pragma HLS ARRAY_PARTITION variable=weight5 complete dim=2
#pragma HLS ARRAY_PARTITION variable=weight5 complete dim=3

    ap_uint<1> weight6[32][IC / IC_VEC][IC_VEC];
#pragma HLS ARRAY_PARTITION variable=weight6 complete dim=2
#pragma HLS ARRAY_PARTITION variable=weight6 complete dim=3

    ap_uint<1> weight7[32][IC / IC_VEC][IC_VEC];
#pragma HLS ARRAY_PARTITION variable=weight7 complete dim=2
#pragma HLS ARRAY_PARTITION variable=weight7 complete dim=3

    ap_uint<1> weight8[32][IC / IC_VEC][IC_VEC];
#pragma HLS ARRAY_PARTITION variable=weight8 complete dim=2
#pragma HLS ARRAY_PARTITION variable=weight8 complete dim=3

    ap_uint<1> weight9[32][IC / IC_VEC][IC_VEC];
#pragma HLS ARRAY_PARTITION variable=weight9 complete dim=2
#pragma HLS ARRAY_PARTITION variable=weight9 complete dim=3

    ap_uint<1> weight10[32][IC / IC_VEC][IC_VEC];
#pragma HLS ARRAY_PARTITION variable=weight10 complete dim=2
#pragma HLS ARRAY_PARTITION variable=weight10 complete dim=3

    ap_uint<1> weight11[32][IC / IC_VEC][IC_VEC];
#pragma HLS ARRAY_PARTITION variable=weight11 complete dim=2
#pragma HLS ARRAY_PARTITION variable=weight11 complete dim=3

	//    for(int i=0; i<(TOTAL_OC/OC); i++){
	//        for(int h=0; h<HEIGHT; h++){

	ap_uint<13> threshold_local[OUT_ACT_NUM];
#pragma HLS ARRAY_PARTITION variable=threshold_local complete dim=0

#pragma HLS ARRAY_PARTITION variable=threshold_local complete dim=0
	for (int i = 0; i < OUT_ACT_NUM; i++) {
#pragma HLS UNROLL
		threshold_local[i] = threshold[0].range(13 * i + 12, 13 * i);
	}

    //int h_iter_in[TOTAL_OC*HEIGHT*TOTAL_IC/OC/IC];
    //int ic_t_iter_in[TOTAL_OC*HEIGHT*TOTAL_IC/OC/IC];
//    int count=0;
//    for(int i=0; i<(TOTAL_OC/OC); i++){
//        for(int h=0; h<HEIGHT; h++){
//            for(int j=0; j<(TOTAL_IC/IC); j++){
//                //h_iter_in[count]=h;
//                //ic_t_iter_in[count]=j;
//                count++;
//            }
//        }
//    }
//
//    int h_iter_out[TOTAL_OC*HEIGHT/OC];
//    int oc_t_iter_out[TOTAL_OC*HEIGHT/OC];
//    int count=0;
//    for(int i=0; i<(TOTAL_OC/OC); i++){
//        for(int h=0; h<HEIGHT; h++){
//            h_iter_out[count]=h;
//            oc_t_iter_out[count]=i;
//            count++;
//        }
//    }

    int count_in=0, count_out=0;
    int h_iter_in=0, ic_t_iter_in=0;
    int oc_t_iter_out = 0;
    int TOTAL_OC_HEIGHT = oc_size * in_fmap_h;
	bool read_weight=true;
	for (int index_out = 0, j = -1; index_out < TOTAL_OC_HEIGHT + 2;) {
		// j is ic_t dimension
		if (j == (TOTAL_IC / IC - 1)) {
			h_iter_in ++;
			ic_t_iter_in=0;
		}
		ap_uint<16> index_in = j + index_out * ic_size;
		bool last_iter = ((j == (ic_size - 1))
				& (index_out == (TOTAL_OC_HEIGHT - 1)));
		bool not_last_out_iter = (index_out < TOTAL_OC_HEIGHT);
		// shouldn't be doing for the last iteration
		if ((index_in[0] == 0) & (!last_iter) & (not_last_out_iter))
            fetch_in_fmap(in_fmap, in_fmap_local1, in_fmap_h, ic_size, h_iter_in % in_fmap_h, ic_t_iter_in, 0);
		else if ((index_in[0] == 1) & (!last_iter) & (not_last_out_iter))
            fetch_in_fmap(in_fmap, in_fmap_local0, in_fmap_h, ic_size, h_iter_in % in_fmap_h, ic_t_iter_in, 0);
        count_in++;
		if ((j >= 0) & not_last_out_iter) {
			if(read_weight){
				weight_S2BRAM(weight_stream, weight0, weight1, weight2,
						weight3, weight4, weight5, weight6, weight7, weight8,
						weight9, weight10, weight11, ic_size);
				read_weight=false;
			}

			for (int w = 0; w < in_fmap_w; w++) {

				if (index_out % 4 == 0) {

					// load partial sum from bram or init with 0
					for (int k = 0; k < OC; k++) {
#pragma HLS UNROLL
						if (j == 0) {
							for (int l = 0; l < N; l++)
#pragma HLS UNROLL
								partial_sum_local[k][l] = 0;
						} else {
							ap_uint<36> partial_sum_vec0 =
									partial_sum_bram0[w][0][k];
							ap_uint<36> partial_sum_vec1 =
									partial_sum_bram0[w][1][k];
							ap_uint<36> partial_sum_vec2 =
									partial_sum_bram0[w][2][k];
							ap_uint<36> partial_sum_vec3 =
									partial_sum_bram0[w][3][k];

							partial_sum_local[k][0].range(3, 0) =
									partial_sum_vec0.range(3, 0);
							partial_sum_local[k][0].range(7, 4) =
									partial_sum_vec1.range(3, 0);
							partial_sum_local[k][0].range(11, 8) =
									partial_sum_vec2.range(3, 0);
							partial_sum_local[k][0].range(12, 12) =
									partial_sum_vec3.range(0, 0);

							partial_sum_local[k][1].range(3, 0) =
									partial_sum_vec0.range(7, 4);
							partial_sum_local[k][1].range(7, 4) =
									partial_sum_vec1.range(7, 4);
							partial_sum_local[k][1].range(11, 8) =
									partial_sum_vec2.range(7, 4);
							partial_sum_local[k][1].range(12, 12) =
									partial_sum_vec3.range(4, 4);

							partial_sum_local[k][2].range(3, 0) =
									partial_sum_vec0.range(11, 8);
							partial_sum_local[k][2].range(7, 4) =
									partial_sum_vec1.range(11, 8);
							partial_sum_local[k][2].range(11, 8) =
									partial_sum_vec2.range(11, 8);
							partial_sum_local[k][2].range(12, 12) =
									partial_sum_vec3.range(8, 8);

							partial_sum_local[k][3].range(3, 0) =
									partial_sum_vec0.range(15, 12);
							partial_sum_local[k][3].range(7, 4) =
									partial_sum_vec1.range(15, 12);
							partial_sum_local[k][3].range(11, 8) =
									partial_sum_vec2.range(15, 12);
							partial_sum_local[k][3].range(12, 12) =
									partial_sum_vec3.range(12, 12);

							partial_sum_local[k][4].range(3, 0) =
									partial_sum_vec0.range(19, 16);
							partial_sum_local[k][4].range(7, 4) =
									partial_sum_vec1.range(19, 16);
							partial_sum_local[k][4].range(11, 8) =
									partial_sum_vec2.range(19, 16);
							partial_sum_local[k][4].range(12, 12) =
									partial_sum_vec3.range(16, 16);

							partial_sum_local[k][5].range(3, 0) =
									partial_sum_vec0.range(23, 20);
							partial_sum_local[k][5].range(7, 4) =
									partial_sum_vec1.range(23, 20);
							partial_sum_local[k][5].range(11, 8) =
									partial_sum_vec2.range(23, 20);
							partial_sum_local[k][5].range(12, 12) =
									partial_sum_vec3.range(20, 20);

							partial_sum_local[k][6].range(3, 0) =
									partial_sum_vec0.range(27, 24);
							partial_sum_local[k][6].range(7, 4) =
									partial_sum_vec1.range(27, 24);
							partial_sum_local[k][6].range(11, 8) =
									partial_sum_vec2.range(27, 24);
							partial_sum_local[k][6].range(12, 12) =
									partial_sum_vec3.range(24, 24);

							partial_sum_local[k][7].range(3, 0) =
									partial_sum_vec0.range(31, 28);
							partial_sum_local[k][7].range(7, 4) =
									partial_sum_vec1.range(31, 28);
							partial_sum_local[k][7].range(11, 8) =
									partial_sum_vec2.range(31, 28);
							partial_sum_local[k][7].range(12, 12) =
									partial_sum_vec3.range(28, 28);

						}
					}
				} else if (index_out%4==1) {
					// load partial sum from bram or init with 0
					for (int k = 0; k < OC; k++) {
#pragma HLS UNROLL
						if (j == 0) {
							for (int l = 0; l < N; l++)
#pragma HLS UNROLL
								partial_sum_local[k][l] = 0;
						} else {
							ap_uint<36> partial_sum_vec0 =
									partial_sum_bram1[w][0][k];
							ap_uint<36> partial_sum_vec1 =
									partial_sum_bram1[w][1][k];
							ap_uint<36> partial_sum_vec2 =
									partial_sum_bram1[w][2][k];
							ap_uint<36> partial_sum_vec3 =
									partial_sum_bram1[w][3][k];

							partial_sum_local[k][0].range(3, 0) =
									partial_sum_vec0.range(3, 0);
							partial_sum_local[k][0].range(7, 4) =
									partial_sum_vec1.range(3, 0);
							partial_sum_local[k][0].range(11, 8) =
									partial_sum_vec2.range(3, 0);
							partial_sum_local[k][0].range(12, 12) =
									partial_sum_vec3.range(0, 0);

							partial_sum_local[k][1].range(3, 0) =
									partial_sum_vec0.range(7, 4);
							partial_sum_local[k][1].range(7, 4) =
									partial_sum_vec1.range(7, 4);
							partial_sum_local[k][1].range(11, 8) =
									partial_sum_vec2.range(7, 4);
							partial_sum_local[k][1].range(12, 12) =
									partial_sum_vec3.range(4, 4);

							partial_sum_local[k][2].range(3, 0) =
									partial_sum_vec0.range(11, 8);
							partial_sum_local[k][2].range(7, 4) =
									partial_sum_vec1.range(11, 8);
							partial_sum_local[k][2].range(11, 8) =
									partial_sum_vec2.range(11, 8);
							partial_sum_local[k][2].range(12, 12) =
									partial_sum_vec3.range(8, 8);

							partial_sum_local[k][3].range(3, 0) =
									partial_sum_vec0.range(15, 12);
							partial_sum_local[k][3].range(7, 4) =
									partial_sum_vec1.range(15, 12);
							partial_sum_local[k][3].range(11, 8) =
									partial_sum_vec2.range(15, 12);
							partial_sum_local[k][3].range(12, 12) =
									partial_sum_vec3.range(12, 12);

							partial_sum_local[k][4].range(3, 0) =
									partial_sum_vec0.range(19, 16);
							partial_sum_local[k][4].range(7, 4) =
									partial_sum_vec1.range(19, 16);
							partial_sum_local[k][4].range(11, 8) =
									partial_sum_vec2.range(19, 16);
							partial_sum_local[k][4].range(12, 12) =
									partial_sum_vec3.range(16, 16);

							partial_sum_local[k][5].range(3, 0) =
									partial_sum_vec0.range(23, 20);
							partial_sum_local[k][5].range(7, 4) =
									partial_sum_vec1.range(23, 20);
							partial_sum_local[k][5].range(11, 8) =
									partial_sum_vec2.range(23, 20);
							partial_sum_local[k][5].range(12, 12) =
									partial_sum_vec3.range(20, 20);

							partial_sum_local[k][6].range(3, 0) =
									partial_sum_vec0.range(27, 24);
							partial_sum_local[k][6].range(7, 4) =
									partial_sum_vec1.range(27, 24);
							partial_sum_local[k][6].range(11, 8) =
									partial_sum_vec2.range(27, 24);
							partial_sum_local[k][6].range(12, 12) =
									partial_sum_vec3.range(24, 24);

							partial_sum_local[k][7].range(3, 0) =
									partial_sum_vec0.range(31, 28);
							partial_sum_local[k][7].range(7, 4) =
									partial_sum_vec1.range(31, 28);
							partial_sum_local[k][7].range(11, 8) =
									partial_sum_vec2.range(31, 28);
							partial_sum_local[k][7].range(12, 12) =
									partial_sum_vec3.range(28, 28);

						}
					}
				} else if (index_out%4==2) {
					for (int k = 0; k < OC; k++) {
#pragma HLS UNROLL
						if (j == 0) {
							for (int l = 0; l < N; l++)
#pragma HLS UNROLL
								partial_sum_local[k][l] = 0;
						} else {
							ap_uint<36> partial_sum_vec0 =
									partial_sum_bram2[w][0][k];
							ap_uint<36> partial_sum_vec1 =
									partial_sum_bram2[w][1][k];
							ap_uint<36> partial_sum_vec2 =
									partial_sum_bram2[w][2][k];
							ap_uint<36> partial_sum_vec3 =
									partial_sum_bram2[w][3][k];

							partial_sum_local[k][0].range(3, 0) =
									partial_sum_vec0.range(3, 0);
							partial_sum_local[k][0].range(7, 4) =
									partial_sum_vec1.range(3, 0);
							partial_sum_local[k][0].range(11, 8) =
									partial_sum_vec2.range(3, 0);
							partial_sum_local[k][0].range(12, 12) =
									partial_sum_vec3.range(0, 0);

							partial_sum_local[k][1].range(3, 0) =
									partial_sum_vec0.range(7, 4);
							partial_sum_local[k][1].range(7, 4) =
									partial_sum_vec1.range(7, 4);
							partial_sum_local[k][1].range(11, 8) =
									partial_sum_vec2.range(7, 4);
							partial_sum_local[k][1].range(12, 12) =
									partial_sum_vec3.range(4, 4);

							partial_sum_local[k][2].range(3, 0) =
									partial_sum_vec0.range(11, 8);
							partial_sum_local[k][2].range(7, 4) =
									partial_sum_vec1.range(11, 8);
							partial_sum_local[k][2].range(11, 8) =
									partial_sum_vec2.range(11, 8);
							partial_sum_local[k][2].range(12, 12) =
									partial_sum_vec3.range(8, 8);

							partial_sum_local[k][3].range(3, 0) =
									partial_sum_vec0.range(15, 12);
							partial_sum_local[k][3].range(7, 4) =
									partial_sum_vec1.range(15, 12);
							partial_sum_local[k][3].range(11, 8) =
									partial_sum_vec2.range(15, 12);
							partial_sum_local[k][3].range(12, 12) =
									partial_sum_vec3.range(12, 12);

							partial_sum_local[k][4].range(3, 0) =
									partial_sum_vec0.range(19, 16);
							partial_sum_local[k][4].range(7, 4) =
									partial_sum_vec1.range(19, 16);
							partial_sum_local[k][4].range(11, 8) =
									partial_sum_vec2.range(19, 16);
							partial_sum_local[k][4].range(12, 12) =
									partial_sum_vec3.range(16, 16);

							partial_sum_local[k][5].range(3, 0) =
									partial_sum_vec0.range(23, 20);
							partial_sum_local[k][5].range(7, 4) =
									partial_sum_vec1.range(23, 20);
							partial_sum_local[k][5].range(11, 8) =
									partial_sum_vec2.range(23, 20);
							partial_sum_local[k][5].range(12, 12) =
									partial_sum_vec3.range(20, 20);

							partial_sum_local[k][6].range(3, 0) =
									partial_sum_vec0.range(27, 24);
							partial_sum_local[k][6].range(7, 4) =
									partial_sum_vec1.range(27, 24);
							partial_sum_local[k][6].range(11, 8) =
									partial_sum_vec2.range(27, 24);
							partial_sum_local[k][6].range(12, 12) =
									partial_sum_vec3.range(24, 24);

							partial_sum_local[k][7].range(3, 0) =
									partial_sum_vec0.range(31, 28);
							partial_sum_local[k][7].range(7, 4) =
									partial_sum_vec1.range(31, 28);
							partial_sum_local[k][7].range(11, 8) =
									partial_sum_vec2.range(31, 28);
							partial_sum_local[k][7].range(12, 12) =
									partial_sum_vec3.range(28, 28);

						}
					}

				} else {
					for (int k = 0; k < OC; k++) {
#pragma HLS UNROLL
						if (j == 0) {
							for (int l = 0; l < N; l++)
#pragma HLS UNROLL
								partial_sum_local[k][l] = 0;
						} else {
							ap_uint<36> partial_sum_vec0 =
									partial_sum_bram3[w][0][k];
							ap_uint<36> partial_sum_vec1 =
									partial_sum_bram3[w][1][k];
							ap_uint<36> partial_sum_vec2 =
									partial_sum_bram3[w][2][k];
							ap_uint<36> partial_sum_vec3 =
									partial_sum_bram3[w][3][k];

							partial_sum_local[k][0].range(3, 0) =
									partial_sum_vec0.range(3, 0);
							partial_sum_local[k][0].range(7, 4) =
									partial_sum_vec1.range(3, 0);
							partial_sum_local[k][0].range(11, 8) =
									partial_sum_vec2.range(3, 0);
							partial_sum_local[k][0].range(12, 12) =
									partial_sum_vec3.range(0, 0);

							partial_sum_local[k][1].range(3, 0) =
									partial_sum_vec0.range(7, 4);
							partial_sum_local[k][1].range(7, 4) =
									partial_sum_vec1.range(7, 4);
							partial_sum_local[k][1].range(11, 8) =
									partial_sum_vec2.range(7, 4);
							partial_sum_local[k][1].range(12, 12) =
									partial_sum_vec3.range(4, 4);

							partial_sum_local[k][2].range(3, 0) =
									partial_sum_vec0.range(11, 8);
							partial_sum_local[k][2].range(7, 4) =
									partial_sum_vec1.range(11, 8);
							partial_sum_local[k][2].range(11, 8) =
									partial_sum_vec2.range(11, 8);
							partial_sum_local[k][2].range(12, 12) =
									partial_sum_vec3.range(8, 8);

							partial_sum_local[k][3].range(3, 0) =
									partial_sum_vec0.range(15, 12);
							partial_sum_local[k][3].range(7, 4) =
									partial_sum_vec1.range(15, 12);
							partial_sum_local[k][3].range(11, 8) =
									partial_sum_vec2.range(15, 12);
							partial_sum_local[k][3].range(12, 12) =
									partial_sum_vec3.range(12, 12);

							partial_sum_local[k][4].range(3, 0) =
									partial_sum_vec0.range(19, 16);
							partial_sum_local[k][4].range(7, 4) =
									partial_sum_vec1.range(19, 16);
							partial_sum_local[k][4].range(11, 8) =
									partial_sum_vec2.range(19, 16);
							partial_sum_local[k][4].range(12, 12) =
									partial_sum_vec3.range(16, 16);

							partial_sum_local[k][5].range(3, 0) =
									partial_sum_vec0.range(23, 20);
							partial_sum_local[k][5].range(7, 4) =
									partial_sum_vec1.range(23, 20);
							partial_sum_local[k][5].range(11, 8) =
									partial_sum_vec2.range(23, 20);
							partial_sum_local[k][5].range(12, 12) =
									partial_sum_vec3.range(20, 20);

							partial_sum_local[k][6].range(3, 0) =
									partial_sum_vec0.range(27, 24);
							partial_sum_local[k][6].range(7, 4) =
									partial_sum_vec1.range(27, 24);
							partial_sum_local[k][6].range(11, 8) =
									partial_sum_vec2.range(27, 24);
							partial_sum_local[k][6].range(12, 12) =
									partial_sum_vec3.range(24, 24);

							partial_sum_local[k][7].range(3, 0) =
									partial_sum_vec0.range(31, 28);
							partial_sum_local[k][7].range(7, 4) =
									partial_sum_vec1.range(31, 28);
							partial_sum_local[k][7].range(11, 8) =
									partial_sum_vec2.range(31, 28);
							partial_sum_local[k][7].range(12, 12) =
									partial_sum_vec3.range(28, 28);

						}
					}
				}

				if (index_in[0] == 0) {
					conv(&(in_fmap_local0[w * (IC / IC_VEC)]), weight0[j], weight1[j],
							weight2[j], weight3[j], weight4[j], weight5[j], weight6[j],
							weight7[j], weight8[j], weight9[j],weight10[j], weight11[j],
							partial_sum_local);
				} else {
					conv(&(in_fmap_local1[w * (IC / IC_VEC)]), weight0[j], weight1[j],
							weight2[j], weight3[j], weight4[j], weight5[j], weight6[j],
							weight7[j], weight8[j], weight9[j],weight10[j], weight11[j],
							partial_sum_local);
				}

				if (index_out % 4 == 0) {

					// store partial sum into bram
					for (int k = 0; k < OC; k++) {
#pragma HLS UNROLL
						ap_uint<36> partial_sum_vec0;
						ap_uint<36> partial_sum_vec1;
						ap_uint<36> partial_sum_vec2;
						ap_uint<36> partial_sum_vec3;

						partial_sum_vec0.range(35, 32) = 0;
						partial_sum_vec1.range(35, 32) = 0;
						partial_sum_vec2.range(35, 32) = 0;
						partial_sum_vec3.range(35, 32) = 0;

						partial_sum_vec0.range(3, 0) =
								partial_sum_local[k][0].range(3, 0);
						partial_sum_vec1.range(3, 0) =
								partial_sum_local[k][0].range(7, 4);
						partial_sum_vec2.range(3, 0) =
								partial_sum_local[k][0].range(11, 8);
						partial_sum_vec3.range(0, 0) =
								partial_sum_local[k][0].range(12, 12);
						partial_sum_vec3.range(3, 1) = 0;

						partial_sum_vec0.range(7, 4) =
								partial_sum_local[k][1].range(3, 0);
						partial_sum_vec1.range(7, 4) =
								partial_sum_local[k][1].range(7, 4);
						partial_sum_vec2.range(7, 4) =
								partial_sum_local[k][1].range(11, 8);
						partial_sum_vec3.range(4, 4) =
								partial_sum_local[k][1].range(12, 12);
						partial_sum_vec3.range(7, 5) = 0;

						partial_sum_vec0.range(11, 8) =
								partial_sum_local[k][2].range(3, 0);
						partial_sum_vec1.range(11, 8) =
								partial_sum_local[k][2].range(7, 4);
						partial_sum_vec2.range(11, 8) =
								partial_sum_local[k][2].range(11, 8);
						partial_sum_vec3.range(8, 8) =
								partial_sum_local[k][2].range(12, 12);
						partial_sum_vec3.range(11, 9) = 0;

						partial_sum_vec0.range(15, 12) =
								partial_sum_local[k][3].range(3, 0);
						partial_sum_vec1.range(15, 12) =
								partial_sum_local[k][3].range(7, 4);
						partial_sum_vec2.range(15, 12) =
								partial_sum_local[k][3].range(11, 8);
						partial_sum_vec3.range(12, 12) =
								partial_sum_local[k][3].range(12, 12);
						partial_sum_vec3.range(15, 13) = 0;

						partial_sum_vec0.range(19, 16) =
								partial_sum_local[k][4].range(3, 0);
						partial_sum_vec1.range(19, 16) =
								partial_sum_local[k][4].range(7, 4);
						partial_sum_vec2.range(19, 16) =
								partial_sum_local[k][4].range(11, 8);
						partial_sum_vec3.range(16, 16) =
								partial_sum_local[k][4].range(12, 12);
						partial_sum_vec3.range(19, 17) = 0;

						partial_sum_vec0.range(23, 20) =
								partial_sum_local[k][5].range(3, 0);
						partial_sum_vec1.range(23, 20) =
								partial_sum_local[k][5].range(7, 4);
						partial_sum_vec2.range(23, 20) =
								partial_sum_local[k][5].range(11, 8);
						partial_sum_vec3.range(20, 20) =
								partial_sum_local[k][5].range(12, 12);
						partial_sum_vec3.range(23, 21) = 0;

						partial_sum_vec0.range(27, 24) =
								partial_sum_local[k][6].range(3, 0);
						partial_sum_vec1.range(27, 24) =
								partial_sum_local[k][6].range(7, 4);
						partial_sum_vec2.range(27, 24) =
								partial_sum_local[k][6].range(11, 8);
						partial_sum_vec3.range(24, 24) =
								partial_sum_local[k][6].range(12, 12);
						partial_sum_vec3.range(27, 25) = 0;

						partial_sum_vec0.range(31, 28) =
								partial_sum_local[k][7].range(3, 0);
						partial_sum_vec1.range(31, 28) =
								partial_sum_local[k][7].range(7, 4);
						partial_sum_vec2.range(31, 28) =
								partial_sum_local[k][7].range(11, 8);
						partial_sum_vec3.range(28, 28) =
								partial_sum_local[k][7].range(12, 12);
						partial_sum_vec3.range(31, 29) = 0;

						partial_sum_bram0[w][0][k] = partial_sum_vec0;
						partial_sum_bram0[w][1][k] = partial_sum_vec1;
						partial_sum_bram0[w][2][k] = partial_sum_vec2;
						partial_sum_bram0[w][3][k] = partial_sum_vec3;
					}
				} else if (index_out%4==1) {

					// store partial sum into bram
					for (int k = 0; k < OC; k++) {
#pragma HLS UNROLL
						ap_uint<36> partial_sum_vec0;
						ap_uint<36> partial_sum_vec1;
						ap_uint<36> partial_sum_vec2;
						ap_uint<36> partial_sum_vec3;

						partial_sum_vec0.range(35, 32) = 0;
						partial_sum_vec1.range(35, 32) = 0;
						partial_sum_vec2.range(35, 32) = 0;
						partial_sum_vec3.range(35, 32) = 0;

						partial_sum_vec0.range(3, 0) =
								partial_sum_local[k][0].range(3, 0);
						partial_sum_vec1.range(3, 0) =
								partial_sum_local[k][0].range(7, 4);
						partial_sum_vec2.range(3, 0) =
								partial_sum_local[k][0].range(11, 8);
						partial_sum_vec3.range(0, 0) =
								partial_sum_local[k][0].range(12, 12);
						partial_sum_vec3.range(3, 1) = 0;

						partial_sum_vec0.range(7, 4) =
								partial_sum_local[k][1].range(3, 0);
						partial_sum_vec1.range(7, 4) =
								partial_sum_local[k][1].range(7, 4);
						partial_sum_vec2.range(7, 4) =
								partial_sum_local[k][1].range(11, 8);
						partial_sum_vec3.range(4, 4) =
								partial_sum_local[k][1].range(12, 12);
						partial_sum_vec3.range(7, 5) = 0;

						partial_sum_vec0.range(11, 8) =
								partial_sum_local[k][2].range(3, 0);
						partial_sum_vec1.range(11, 8) =
								partial_sum_local[k][2].range(7, 4);
						partial_sum_vec2.range(11, 8) =
								partial_sum_local[k][2].range(11, 8);
						partial_sum_vec3.range(8, 8) =
								partial_sum_local[k][2].range(12, 12);
						partial_sum_vec3.range(11, 9) = 0;

						partial_sum_vec0.range(15, 12) =
								partial_sum_local[k][3].range(3, 0);
						partial_sum_vec1.range(15, 12) =
								partial_sum_local[k][3].range(7, 4);
						partial_sum_vec2.range(15, 12) =
								partial_sum_local[k][3].range(11, 8);
						partial_sum_vec3.range(12, 12) =
								partial_sum_local[k][3].range(12, 12);
						partial_sum_vec3.range(15, 13) = 0;

						partial_sum_vec0.range(19, 16) =
								partial_sum_local[k][4].range(3, 0);
						partial_sum_vec1.range(19, 16) =
								partial_sum_local[k][4].range(7, 4);
						partial_sum_vec2.range(19, 16) =
								partial_sum_local[k][4].range(11, 8);
						partial_sum_vec3.range(16, 16) =
								partial_sum_local[k][4].range(12, 12);
						partial_sum_vec3.range(19, 17) = 0;

						partial_sum_vec0.range(23, 20) =
								partial_sum_local[k][5].range(3, 0);
						partial_sum_vec1.range(23, 20) =
								partial_sum_local[k][5].range(7, 4);
						partial_sum_vec2.range(23, 20) =
								partial_sum_local[k][5].range(11, 8);
						partial_sum_vec3.range(20, 20) =
								partial_sum_local[k][5].range(12, 12);
						partial_sum_vec3.range(23, 21) = 0;

						partial_sum_vec0.range(27, 24) =
								partial_sum_local[k][6].range(3, 0);
						partial_sum_vec1.range(27, 24) =
								partial_sum_local[k][6].range(7, 4);
						partial_sum_vec2.range(27, 24) =
								partial_sum_local[k][6].range(11, 8);
						partial_sum_vec3.range(24, 24) =
								partial_sum_local[k][6].range(12, 12);
						partial_sum_vec3.range(27, 25) = 0;

						partial_sum_vec0.range(31, 28) =
								partial_sum_local[k][7].range(3, 0);
						partial_sum_vec1.range(31, 28) =
								partial_sum_local[k][7].range(7, 4);
						partial_sum_vec2.range(31, 28) =
								partial_sum_local[k][7].range(11, 8);
						partial_sum_vec3.range(28, 28) =
								partial_sum_local[k][7].range(12, 12);
						partial_sum_vec3.range(31, 29) = 0;

						partial_sum_bram1[w][0][k] = partial_sum_vec0;
						partial_sum_bram1[w][1][k] = partial_sum_vec1;
						partial_sum_bram1[w][2][k] = partial_sum_vec2;
						partial_sum_bram1[w][3][k] = partial_sum_vec3;

					}
				} else if (index_out%4==2) {
					for (int k = 0; k < OC; k++) {
#pragma HLS UNROLL
						ap_uint<36> partial_sum_vec0;
						ap_uint<36> partial_sum_vec1;
						ap_uint<36> partial_sum_vec2;
						ap_uint<36> partial_sum_vec3;

						partial_sum_vec0.range(35, 32) = 0;
						partial_sum_vec1.range(35, 32) = 0;
						partial_sum_vec2.range(35, 32) = 0;
						partial_sum_vec3.range(35, 32) = 0;

						partial_sum_vec0.range(3, 0) =
								partial_sum_local[k][0].range(3, 0);
						partial_sum_vec1.range(3, 0) =
								partial_sum_local[k][0].range(7, 4);
						partial_sum_vec2.range(3, 0) =
								partial_sum_local[k][0].range(11, 8);
						partial_sum_vec3.range(0, 0) =
								partial_sum_local[k][0].range(12, 12);
						partial_sum_vec3.range(3, 1) = 0;

						partial_sum_vec0.range(7, 4) =
								partial_sum_local[k][1].range(3, 0);
						partial_sum_vec1.range(7, 4) =
								partial_sum_local[k][1].range(7, 4);
						partial_sum_vec2.range(7, 4) =
								partial_sum_local[k][1].range(11, 8);
						partial_sum_vec3.range(4, 4) =
								partial_sum_local[k][1].range(12, 12);
						partial_sum_vec3.range(7, 5) = 0;

						partial_sum_vec0.range(11, 8) =
								partial_sum_local[k][2].range(3, 0);
						partial_sum_vec1.range(11, 8) =
								partial_sum_local[k][2].range(7, 4);
						partial_sum_vec2.range(11, 8) =
								partial_sum_local[k][2].range(11, 8);
						partial_sum_vec3.range(8, 8) =
								partial_sum_local[k][2].range(12, 12);
						partial_sum_vec3.range(11, 9) = 0;

						partial_sum_vec0.range(15, 12) =
								partial_sum_local[k][3].range(3, 0);
						partial_sum_vec1.range(15, 12) =
								partial_sum_local[k][3].range(7, 4);
						partial_sum_vec2.range(15, 12) =
								partial_sum_local[k][3].range(11, 8);
						partial_sum_vec3.range(12, 12) =
								partial_sum_local[k][3].range(12, 12);
						partial_sum_vec3.range(15, 13) = 0;

						partial_sum_vec0.range(19, 16) =
								partial_sum_local[k][4].range(3, 0);
						partial_sum_vec1.range(19, 16) =
								partial_sum_local[k][4].range(7, 4);
						partial_sum_vec2.range(19, 16) =
								partial_sum_local[k][4].range(11, 8);
						partial_sum_vec3.range(16, 16) =
								partial_sum_local[k][4].range(12, 12);
						partial_sum_vec3.range(19, 17) = 0;

						partial_sum_vec0.range(23, 20) =
								partial_sum_local[k][5].range(3, 0);
						partial_sum_vec1.range(23, 20) =
								partial_sum_local[k][5].range(7, 4);
						partial_sum_vec2.range(23, 20) =
								partial_sum_local[k][5].range(11, 8);
						partial_sum_vec3.range(20, 20) =
								partial_sum_local[k][5].range(12, 12);
						partial_sum_vec3.range(23, 21) = 0;

						partial_sum_vec0.range(27, 24) =
								partial_sum_local[k][6].range(3, 0);
						partial_sum_vec1.range(27, 24) =
								partial_sum_local[k][6].range(7, 4);
						partial_sum_vec2.range(27, 24) =
								partial_sum_local[k][6].range(11, 8);
						partial_sum_vec3.range(24, 24) =
								partial_sum_local[k][6].range(12, 12);
						partial_sum_vec3.range(27, 25) = 0;

						partial_sum_vec0.range(31, 28) =
								partial_sum_local[k][7].range(3, 0);
						partial_sum_vec1.range(31, 28) =
								partial_sum_local[k][7].range(7, 4);
						partial_sum_vec2.range(31, 28) =
								partial_sum_local[k][7].range(11, 8);
						partial_sum_vec3.range(28, 28) =
								partial_sum_local[k][7].range(12, 12);
						partial_sum_vec3.range(31, 29) = 0;

						partial_sum_bram2[w][0][k] = partial_sum_vec0;
						partial_sum_bram2[w][1][k] = partial_sum_vec1;
						partial_sum_bram2[w][2][k] = partial_sum_vec2;
						partial_sum_bram2[w][3][k] = partial_sum_vec3;

					}
				} else {
					for (int k = 0; k < OC; k++) {
#pragma HLS UNROLL
						ap_uint<36> partial_sum_vec0;
						ap_uint<36> partial_sum_vec1;
						ap_uint<36> partial_sum_vec2;
						ap_uint<36> partial_sum_vec3;

						partial_sum_vec0.range(35, 32) = 0;
						partial_sum_vec1.range(35, 32) = 0;
						partial_sum_vec2.range(35, 32) = 0;
						partial_sum_vec3.range(35, 32) = 0;

						partial_sum_vec0.range(3, 0) =
								partial_sum_local[k][0].range(3, 0);
						partial_sum_vec1.range(3, 0) =
								partial_sum_local[k][0].range(7, 4);
						partial_sum_vec2.range(3, 0) =
								partial_sum_local[k][0].range(11, 8);
						partial_sum_vec3.range(0, 0) =
								partial_sum_local[k][0].range(12, 12);
						partial_sum_vec3.range(3, 1) = 0;

						partial_sum_vec0.range(7, 4) =
								partial_sum_local[k][1].range(3, 0);
						partial_sum_vec1.range(7, 4) =
								partial_sum_local[k][1].range(7, 4);
						partial_sum_vec2.range(7, 4) =
								partial_sum_local[k][1].range(11, 8);
						partial_sum_vec3.range(4, 4) =
								partial_sum_local[k][1].range(12, 12);
						partial_sum_vec3.range(7, 5) = 0;

						partial_sum_vec0.range(11, 8) =
								partial_sum_local[k][2].range(3, 0);
						partial_sum_vec1.range(11, 8) =
								partial_sum_local[k][2].range(7, 4);
						partial_sum_vec2.range(11, 8) =
								partial_sum_local[k][2].range(11, 8);
						partial_sum_vec3.range(8, 8) =
								partial_sum_local[k][2].range(12, 12);
						partial_sum_vec3.range(11, 9) = 0;

						partial_sum_vec0.range(15, 12) =
								partial_sum_local[k][3].range(3, 0);
						partial_sum_vec1.range(15, 12) =
								partial_sum_local[k][3].range(7, 4);
						partial_sum_vec2.range(15, 12) =
								partial_sum_local[k][3].range(11, 8);
						partial_sum_vec3.range(12, 12) =
								partial_sum_local[k][3].range(12, 12);
						partial_sum_vec3.range(15, 13) = 0;

						partial_sum_vec0.range(19, 16) =
								partial_sum_local[k][4].range(3, 0);
						partial_sum_vec1.range(19, 16) =
								partial_sum_local[k][4].range(7, 4);
						partial_sum_vec2.range(19, 16) =
								partial_sum_local[k][4].range(11, 8);
						partial_sum_vec3.range(16, 16) =
								partial_sum_local[k][4].range(12, 12);
						partial_sum_vec3.range(19, 17) = 0;

						partial_sum_vec0.range(23, 20) =
								partial_sum_local[k][5].range(3, 0);
						partial_sum_vec1.range(23, 20) =
								partial_sum_local[k][5].range(7, 4);
						partial_sum_vec2.range(23, 20) =
								partial_sum_local[k][5].range(11, 8);
						partial_sum_vec3.range(20, 20) =
								partial_sum_local[k][5].range(12, 12);
						partial_sum_vec3.range(23, 21) = 0;

						partial_sum_vec0.range(27, 24) =
								partial_sum_local[k][6].range(3, 0);
						partial_sum_vec1.range(27, 24) =
								partial_sum_local[k][6].range(7, 4);
						partial_sum_vec2.range(27, 24) =
								partial_sum_local[k][6].range(11, 8);
						partial_sum_vec3.range(24, 24) =
								partial_sum_local[k][6].range(12, 12);
						partial_sum_vec3.range(27, 25) = 0;

						partial_sum_vec0.range(31, 28) =
								partial_sum_local[k][7].range(3, 0);
						partial_sum_vec1.range(31, 28) =
								partial_sum_local[k][7].range(7, 4);
						partial_sum_vec2.range(31, 28) =
								partial_sum_local[k][7].range(11, 8);
						partial_sum_vec3.range(28, 28) =
								partial_sum_local[k][7].range(12, 12);
						partial_sum_vec3.range(31, 29) = 0;

						partial_sum_bram3[w][0][k] = partial_sum_vec0;
						partial_sum_bram3[w][1][k] = partial_sum_vec1;
						partial_sum_bram3[w][2][k] = partial_sum_vec2;
						partial_sum_bram3[w][3][k] = partial_sum_vec3;

					}
				}

			}
        }
			
		if ((index_out == TOTAL_OC_HEIGHT)|(index_out == TOTAL_OC_HEIGHT+1)){
			j = (TOTAL_IC / IC - 1);
		}
		if (j == (TOTAL_IC / IC - 1)) {
//            bool first_iter=(i==0)&(h==0);
			bool last_iter = (index_out == TOTAL_OC_HEIGHT);
			bool second_last_iter= (index_out == TOTAL_OC_HEIGHT+1);
			//bool first_iter=(index_out==0);

            if((! last_iter)&(!second_last_iter)) {
				if (index_out % 4 == 3) { // need to exclude the first iteration, and add last iteration 
					//store_out_fmap(partial_sum_bram2, out_fmap, threshold_local, in_fmap_h, oc_size, count_out, oc_t_iter_out, 0, true);
					//store_out_fmap(partial_sum_bram3, out_fmap, threshold_local, in_fmap_h, oc_size, count_out+1, oc_t_iter_out, 0, true);
					store_out_fmap3(partial_sum_bram2, partial_sum_bram3, out_fmap, threshold_local, in_fmap_h, oc_size, count_out, oc_t_iter_out, 0, is_shift, is_pool);
					count_out+=2;
				}
				else if(index_out % 4 == 1) {
					//store_out_fmap(partial_sum_bram0, out_fmap, threshold_local, in_fmap_h, oc_size, count_out, oc_t_iter_out, 0, true);
					//store_out_fmap(partial_sum_bram1, out_fmap, threshold_local, in_fmap_h, oc_size, count_out+1, oc_t_iter_out, 0, true);
					store_out_fmap3(partial_sum_bram0, partial_sum_bram1, out_fmap, threshold_local, in_fmap_h, oc_size, count_out, oc_t_iter_out, 0, is_shift, is_pool);
					count_out+=2;
				}
	                	
				if(count_out == in_fmap_h ){
	                oc_t_iter_out ++;
	                count_out = 0;
					read_weight=true;
	            }

            }

		}

		j++;
		ic_t_iter_in++;

		if (j == TOTAL_IC / IC) {
			j = 0;
			index_out++;
		}
	}
	// How do you know if this is always bram1??
	//out_fmap_BRAM2S(partial_sum_bram1, out_fmap_stream, threshold_local, OC, N,ACT_BIT);

}

void conv(ap_uint<36> in_fmap[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight0[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight1[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight2[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight3[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight4[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight5[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight6[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight7[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight8[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight9[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight10[IC / IC_VEC][IC_VEC],
		ap_uint<1> weight11[IC / IC_VEC][IC_VEC],
		ap_uint<13> partial_sum_local[OC][N]) {

#pragma HLS ARRAY_PARTITION variable=partial_sum_local complete dim=0

	ap_uint<4> in_fmap_local[IC_VEC][N];
#pragma HLS ARRAY_PARTITION variable=in_fmap_local complete dim=0

	conv_1x1_kernel_ic: for (int ic = 0; ic < IC / IC_VEC; ic++) {
#pragma HLS PIPELINE II=1

		ap_uint<32> in_fmap_vec0 = in_fmap[ic][0].range(31, 0);
		ap_uint<32> in_fmap_vec1 = in_fmap[ic][1].range(31, 0);
		ap_uint<32> in_fmap_vec2 = in_fmap[ic][2].range(31, 0);
		ap_uint<32> in_fmap_vec3 = in_fmap[ic][3].range(31, 0);
		ap_uint<32> in_fmap_vec4 = in_fmap[ic][4].range(31, 0);
		ap_uint<32> in_fmap_vec5 = in_fmap[ic][5].range(31, 0);

		conv_1x1_kernel_n: for (int col = 0; col < N; col++) {
#pragma HLS UNROLL

			in_fmap_local[0][col] = in_fmap_vec0.range(col * 4 + 3, col * 4);
			in_fmap_local[1][col] = in_fmap_vec1.range(col * 4 + 3, col * 4);
			in_fmap_local[2][col] = in_fmap_vec2.range(col * 4 + 3, col * 4);
			in_fmap_local[3][col] = in_fmap_vec3.range(col * 4 + 3, col * 4);
			in_fmap_local[4][col] = in_fmap_vec4.range(col * 4 + 3, col * 4);
			in_fmap_local[5][col] = in_fmap_vec5.range(col * 4 + 3, col * 4);

			partial_sum_local[0][col] = partial_sum_local[0][col]
					+ in_fmap_local[0][col] * weight0[ic][0]
					+ in_fmap_local[1][col] * weight0[ic][1]
					+ in_fmap_local[2][col] * weight0[ic][2]
					+ in_fmap_local[3][col] * weight0[ic][3]
					+ in_fmap_local[4][col] * weight0[ic][4]
					+ in_fmap_local[5][col] * weight0[ic][5];					
			partial_sum_local[1][col] = partial_sum_local[1][col]
					+ in_fmap_local[0][col] * weight1[ic][0]
					+ in_fmap_local[1][col] * weight1[ic][1]
					+ in_fmap_local[2][col] * weight1[ic][2]
					+ in_fmap_local[3][col] * weight1[ic][3]
					+ in_fmap_local[4][col] * weight1[ic][4]
					+ in_fmap_local[5][col] * weight1[ic][5];
			partial_sum_local[2][col] = partial_sum_local[2][col]
					+ in_fmap_local[0][col] * weight2[ic][0]
					+ in_fmap_local[1][col] * weight2[ic][1]
					+ in_fmap_local[2][col] * weight2[ic][2]
					+ in_fmap_local[3][col] * weight2[ic][3]
					+ in_fmap_local[4][col] * weight2[ic][4]
					+ in_fmap_local[5][col] * weight2[ic][5];
			partial_sum_local[3][col] = partial_sum_local[3][col]
					+ in_fmap_local[0][col] * weight3[ic][0]
					+ in_fmap_local[1][col] * weight3[ic][1]
					+ in_fmap_local[2][col] * weight3[ic][2]
					+ in_fmap_local[3][col] * weight3[ic][3]
					+ in_fmap_local[4][col] * weight3[ic][4]
					+ in_fmap_local[5][col] * weight3[ic][5];
			partial_sum_local[4][col] = partial_sum_local[4][col]
					+ in_fmap_local[0][col] * weight4[ic][0]
					+ in_fmap_local[1][col] * weight4[ic][1]
					+ in_fmap_local[2][col] * weight4[ic][2]
					+ in_fmap_local[3][col] * weight4[ic][3]
					+ in_fmap_local[4][col] * weight4[ic][4]
					+ in_fmap_local[5][col] * weight4[ic][5];
			partial_sum_local[5][col] = partial_sum_local[5][col]
					+ in_fmap_local[0][col] * weight5[ic][0]
					+ in_fmap_local[1][col] * weight5[ic][1]
					+ in_fmap_local[2][col] * weight5[ic][2]
					+ in_fmap_local[3][col] * weight5[ic][3]
					+ in_fmap_local[4][col] * weight5[ic][4]
					+ in_fmap_local[5][col] * weight5[ic][5];
			partial_sum_local[6][col] = partial_sum_local[6][col]
					+ in_fmap_local[0][col] * weight6[ic][0]
					+ in_fmap_local[1][col] * weight6[ic][1]
					+ in_fmap_local[2][col] * weight6[ic][2]
					+ in_fmap_local[3][col] * weight6[ic][3]
					+ in_fmap_local[4][col] * weight6[ic][4]
					+ in_fmap_local[5][col] * weight6[ic][5];
			partial_sum_local[7][col] = partial_sum_local[7][col]
					+ in_fmap_local[0][col] * weight7[ic][0]
					+ in_fmap_local[1][col] * weight7[ic][1]
					+ in_fmap_local[2][col] * weight7[ic][2]
					+ in_fmap_local[3][col] * weight7[ic][3]
					+ in_fmap_local[4][col] * weight7[ic][4]
					+ in_fmap_local[5][col] * weight7[ic][5];
			partial_sum_local[8][col] = partial_sum_local[8][col]
					+ in_fmap_local[0][col] * weight8[ic][0]
					+ in_fmap_local[1][col] * weight8[ic][1]
					+ in_fmap_local[2][col] * weight8[ic][2]
					+ in_fmap_local[3][col] * weight8[ic][3]
					+ in_fmap_local[4][col] * weight8[ic][4]
					+ in_fmap_local[5][col] * weight8[ic][5];
			partial_sum_local[9][col] = partial_sum_local[9][col]
					+ in_fmap_local[0][col] * weight9[ic][0]
					+ in_fmap_local[1][col] * weight9[ic][1]
					+ in_fmap_local[2][col] * weight9[ic][2]
					+ in_fmap_local[3][col] * weight9[ic][3]
					+ in_fmap_local[4][col] * weight9[ic][4]
					+ in_fmap_local[5][col] * weight9[ic][5];
			partial_sum_local[10][col] = partial_sum_local[10][col]
					+ in_fmap_local[0][col] * weight10[ic][0]
					+ in_fmap_local[1][col] * weight10[ic][1]
					+ in_fmap_local[2][col] * weight10[ic][2]
					+ in_fmap_local[3][col] * weight10[ic][3]
					+ in_fmap_local[4][col] * weight10[ic][4]
					+ in_fmap_local[5][col] * weight10[ic][5];
			partial_sum_local[11][col] = partial_sum_local[11][col]
					+ in_fmap_local[0][col] * weight11[ic][0]
					+ in_fmap_local[1][col] * weight11[ic][1]
					+ in_fmap_local[2][col] * weight11[ic][2]
					+ in_fmap_local[3][col] * weight11[ic][3]
					+ in_fmap_local[4][col] * weight11[ic][4]
					+ in_fmap_local[5][col] * weight11[ic][5];

		}

	}
}
