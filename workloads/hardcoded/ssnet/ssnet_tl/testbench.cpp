#include <stdlib.h>
#include <time.h>
#include <fstream>
#include "shiftshuffle_kernel.h"
using namespace std;

void print_fmap_row2(ap_uint<128> vec){
    for(int i=0;i<32;i++){
        cout<<vec.range(4*i+3,4*i)<<",";
    }
    cout<<endl;
}

void print_weight(ap_uint<1> weight[IC / IC_VEC][IC_VEC]){
    for(int i=0; i<(IC/IC_VEC); i++){
        ap_uint<9> vec;
        for(int j=0; j<9; j++){
            vec.range(j,j)=weight[i][j];
        }
        cout<<vec<<",";
    }
    cout<<endl;
}

int main()
{
    srand(1);
    //   ifstream fin1("in_fmap.txt");
//ifstream fin2("weight.txt");
    ifstream fin1("/scratch/qijing.huang/ShiftShuffleNet/hw/shift_layer/dataflow27/in_fmap.txt");
    ifstream fin2("/scratch/qijing.huang/ShiftShuffleNet/hw/shift_layer/dataflow27/weight.txt");

    
    ap_uint<256> threshold;
    ap_uint<512> weight[TOTAL_OC*TOTAL_IC/OC/IC];
    ap_uint<128> in_fmap[TOTAL_IC*HEIGHT];
    //ap_uint<36> in_fmap_bram[8][9];
    //the first is how many N, second is different bit for 13 bits, third is oc
    //ap_uint<36> partial_sum_bram0[4][4][OC];
    ap_uint<128> out_fmap[TOTAL_OC*HEIGHT];

    ap_uint<4> in_fmap_test[TOTAL_IC][HEIGHT][WIDTH];
    ap_uint<4> out_fmap_test[TOTAL_OC][HEIGHT][WIDTH];
    ap_uint<4> out_fmap_pool[TOTAL_OC][HEIGHT/2][WIDTH/2];

    ap_uint<13> threshold_array[15];
    ap_uint<1> weight_array[TOTAL_OC][TOTAL_IC];
    //ap_uint<13> partial_sum_local[OC][N];
    //ap_uint<13> partial_sum_tb[OC][N];

    //hls::stream<ap_uint<128> > in_fmap_stream;
    //hls::stream<ap_uint<324> > weight_stream;
    //hls::stream<ap_uint<128> > out_fmap_stream;


    for(int i=0; i<15; i++){
        threshold.range(i*13+12,i*13)=(ap_uint<13>)((i+1)*16);
        threshold_array[i]=(ap_uint<13>)((i+1)*16);
    }
    threshold.range(199,195)=0;

    for(int i=0; i<TOTAL_IC; i++){
        for(int j=0; j<HEIGHT; j++){
            for(int k=0; k<WIDTH; k++){
                //int a=rand()%16;
                int a;
                fin1>>a;
                in_fmap[i+j*TOTAL_IC].range(4*k+3,4*k)=(ap_uint<4>)a;
                in_fmap_test[i][j][k]=(ap_uint<4>)a;
            }
        }
    }

    for(int i=0; i<TOTAL_OC/OC; i++){
        for(int j=0; j<TOTAL_IC/IC; j++){
            for(int k=0; k<324; k++){
                int b;
                fin2>>b;
                weight[i*(TOTAL_IC/IC)+j].range(k,k)=b;
            }
            weight[i*(TOTAL_IC/IC)+j].range(327,324)=0;
        }
    }


    for(int i=0; i<TOTAL_OC/OC; i++){
        for(int j=0; j<TOTAL_IC/IC; j++){
            for(int k=0; k<OC; k++){
                for(int l=0; l<IC; l++){
                    weight_array[i*OC+k][j*IC+l]=weight[i*(TOTAL_IC/IC)+j].range(k*IC+l,k*IC+l);
                }
            }
        }
    }


    for(int i=0; i<TOTAL_OC; i++){
        for(int j=0; j<HEIGHT; j++){
            out_fmap[i*HEIGHT+j]=0;
        }
    }

    for(int i=0; i<TOTAL_OC; i++){
        for(int j=0; j<HEIGHT; j++){
            for(int k=0; k<WIDTH; k++){
                out_fmap_test[i][j][k]=0;
            }
        }
    }

    /*for(int i=0; i<OC; i++){
        for(int j=0; j<N; j++){
            partial_sum_local[i][j]=0;
            partial_sum_tb[i][j]=0;
        }
    }*/


    /*for(int h=0; h<HEIGHT; h++){
        for(int i=0; i<OC; i++){
            for(int j=0; j<4; j++){
                for(int k=0; k<8; k++){
                    ap_uint<13> a=(ap_uint<13>)(rand()%8192);
                    out_fmap_test[i][h][j*8+k]=convert(a, threshold_array);

                    out_fmap_bram[j][0][i].range(k*4+3,k*4)=a.range(3,0);
                    out_fmap_bram[j][1][i].range(k*4+3,k*4)=a.range(7,4);
                    out_fmap_bram[j][2][i].range(k*4+3,k*4)=a.range(11,8);
                    out_fmap_bram[j][3][i].range(k*4,k*4)=a.range(12,12);
                    //cout<<convert(a, threshold_array)<<",";
                }
            }
            //cout<<endl;
        }
        out_fmap_BRAM2S(out_fmap_bram, out_fmap_stream, threshold_array, OC, N, ACT_BIT);

    }
    store_out_fmap(out_fmap, out_fmap_stream, TOTAL_OC, OC, HEIGHT);*/

    /*cout<<endl<<"input"<<endl;
    for(int i=0; i<TOTAL_IC; i++){
        for(int j=0; j<HEIGHT; j++){
            ap_uint<128> in_fmap_vec=in_fmap[i*HEIGHT+j];
            if((j==1)&(i%9==2))
                print_fmap_row(in_fmap_vec);
        }
    }*/

    /*for(int i=0; i<18; i++){
        for(int j=0; j<2; j++){
            ap_uint<9> vec;
            for(int k=0; k<9; k++){
                vec.range(k,k)=weight.range(i*18+j*9+k,i*18+j*9+k);
            }
            cout<<vec<<",";
        }
        cout<<endl;
    }*/

    /*ap_uint<1> weight0[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight1[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight2[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight3[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight4[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight5[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight6[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight7[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight8[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight9[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight10[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight11[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight12[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight13[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight14[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight15[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight16[IC / IC_VEC][IC_VEC];
    ap_uint<1> weight17[IC / IC_VEC][IC_VEC];*/

    //fetch_in_fmap(in_fmap, in_fmap_stream, HEIGHT, OC, TOTAL_OC, TOTAL_IC);
    //fetch_weight(&weight, weight_stream, TOTAL_IC, IC, TOTAL_OC, OC, HEIGHT);
    //in_fmap_S2BRAM(in_fmap_stream, in_fmap_bram, N, ACT_BIT, IC, IC_VEC);
    //in_fmap_S2BRAM(in_fmap_stream, in_fmap_bram, N, ACT_BIT, IC, IC_VEC);
    //in_fmap_S2BRAM(in_fmap_stream, in_fmap_bram, N, ACT_BIT, IC, IC_VEC);
    //out_fmap_BRAM2S(out_fmap_bram, out_fmap_stream, threshold_array, OC, N, ACT_BIT);
    //store_out_fmap(ap_uint<128>* out_fmap, out_fmap_stream, TOTAL_OC, OC, HEIGHT);


    /*for(int i=0; i<10; i++){
        ap_uint<13> a=(ap_uint<13>)rand()%256;
        cout<<a<<endl;
        cout<<convert(a,threshold_array)<<endl;
    }*/

    /*int w=3;

    weight_S2BRAM(weight_stream, IC, IC_VEC,
                  weight0,weight1,weight2,weight3,weight4,weight5,
                  weight6,weight7,weight8,weight9,weight10,weight11,
                  weight12,weight13,weight14,weight15,weight16,weight17);

    conv(&(in_fmap_bram[w*(IC/IC_VEC)]),
                             weight0,
                             weight1,
                             weight2,
                             weight3,
                             weight4,
                             weight5,
                             weight6,
                             weight7,
                             weight8,
                             weight9,
                             weight10,
                             weight11,
                             weight12,
                             weight13,
                             weight14,
                             weight15,
                             weight16,
                             weight17,
                             partial_sum_local);*/

    /*for(int i=0; i<OC; i++){
        for(int j=0; j<N; j++){
            cout<<partial_sum_local[i][j]<<endl;
        }
    }*/

    // conv tb
    /*for(int i=0; i<OC; i++){
        for(int j=0; j<N; j++){
            for(int k=0; k< IC; k++){
                partial_sum_tb[i][j]+=
                    (ap_uint<13>)weight.range(k+i*IC,k+i*IC)*(ap_uint<13>)in_fmap[k*HEIGHT+2].range(j*4+3+32*w,j*4+32*w);
            }
        }
    }

    bool same=true;
    for(int i=0; i<OC; i++){
        for(int j=0; j<N; j++){
            if(partial_sum_tb[i][j]!=partial_sum_local[i][j]){
                same=false;
                break;
            }
        }
    }
    cout<<"same:"<<same<<endl;*/

    /*cout<<endl<<"output"<<endl;
    for(int j=0; j<HEIGHT; j++){
        for(int i=0; i<TOTAL_IC; i++){
            ap_uint<128> in_fmap_vec=in_fmap_stream.read();
            if(j==2)
                print_fmap_row(in_fmap_vec);
        }
    }*/

    /*for(int i=0; i<HEIGHT; i++){
        ap_uint<324> weight_vec=weight_stream.read();
        for(int j=0; j<324; j++){
            cout<<weight_vec.range(j,j)<<",";
        }
        cout<<endl;
    }*/

    /*cout<<endl<<"output"<<endl;
    for(int i=0; i<8; i++){
        for(int j=0; j<8; j++){
            cout<<in_fmap_bram[i][2].range(3+j*4, j*4)<<",";
        }
        cout<<endl;
    }*/

    /*cout<<endl<<"result"<<endl;

    print_weight(weight0);
    print_weight(weight1);
    print_weight(weight2);
    print_weight(weight3);
    print_weight(weight4);
    print_weight(weight5);
    print_weight(weight6);
    print_weight(weight7);
    print_weight(weight8);
    print_weight(weight9);
    print_weight(weight10);
    print_weight(weight11);
    print_weight(weight12);
    print_weight(weight13);
    print_weight(weight14);
    print_weight(weight15);
    print_weight(weight16);
    print_weight(weight17);*/

    /*cout<<"hello:"<<endl;
    bool same=true;
    for(int i=0; i<OC; i++){
        ap_uint<128> result_vec=out_fmap_stream.read();
        for(int j=0; j<32; j++){
            ap_uint<4> a=result_vec.range(j*4+3,j*4);
            ap_uint<4> b=convert(out_fmap_test[j][i], threshold_array);
            if(a!=b){
                same=false;
                cout<<endl<<"different in:"<<j<<","<<i<<endl;
                cout<<a<<","<<b<<endl;
                //break;
            }
            cout<<a<<",";
        }
        cout<<endl;
    }
    cout<<"result:"<<same<<endl;*/

    /*bool same=true;
    for(int i=0; i<OC; i++){
        for(int j=0; j<HEIGHT; j++){
            for(int k=0; k<WIDTH; k++){
                ap_uint<4> a=out_fmap[i*HEIGHT+j].range(k*4+3,k*4);
                ap_uint<4> b=out_fmap_test[i][j][k];
                if(a!=b){
                    same=false;
                    cout<<endl<<"different in:"<<i<<","<<j<<","<<k<<endl;
                    cout<<a<<","<<b<<endl;
                    break;
                }
            }
        }
    }
    cout<<"result:"<<same<<endl;*/


    //ap_uint<1> a=1;
    //cout<<a<<endl;

    //ap_uint<1> b=0;
    //cout<<b<<endl;

    //ap_uint<1> c=-1;
    //cout<<c<<endl;
    
    print_fmap_row2(out_fmap[21]);
    for(int i=0; i<WIDTH; i++){
        cout<<out_fmap_test[21][0][i]<<",";
    }
    cout<<endl;

    // in fmap shift
    /*for(int i=0; i<TOTAL_IC; i++){
        if(i%5==0){
            //shift down
            for(int j=HEIGHT-1; j>=1; j--){
                for(int k=0; k<WIDTH; k++){
                    in_fmap_test[i][j][k]=in_fmap_test[i][j-1][k];
                }
            }
            for(int k=0; k<WIDTH; k++){
                in_fmap_test[i][0][k]=0;
            }
        }
        else if(i%5==1){
            //shift right
            for(int j=0; j<HEIGHT; j++){
                for(int k=WIDTH-1; k>=1; k--){
                    in_fmap_test[i][j][k]=in_fmap_test[i][j][k-1];
                }
                in_fmap_test[i][j][0]=0;
            }
        }
        else if(i%5==3){
            //shift left
            for(int j=0; j<HEIGHT; j++){
                for(int k=0; k<=WIDTH-2; k++){
                    in_fmap_test[i][j][k]=in_fmap_test[i][j][k+1];
                }
                in_fmap_test[i][j][WIDTH-1]=0;
            }
        }
        else if(i%5==4){
            //shift up
            for(int j=0; j<=HEIGHT-2; j++){
                for(int k=0; k<WIDTH; k++){
                    in_fmap_test[i][j][k]=in_fmap_test[i][j+1][k];
                }
            }
            for(int k=0; k<WIDTH; k++){
                in_fmap_test[i][HEIGHT-1][k]=0;
            }
        }
    }*/
int in_fmap_w = (WIDTH + N - 1) / N;
int in_fmap_h = HEIGHT;
int ic_size = TOTAL_IC / IC;
int oc_size = TOTAL_OC / OC;

    top(&threshold, in_fmap, weight, out_fmap, in_fmap_w, in_fmap_h, ic_size, oc_size, 1, 1);

    for(int i=0; i<TOTAL_OC; i++){
        for(int j=0; j<HEIGHT; j++){
            for(int k=0; k<WIDTH; k++){
                ap_uint<13> sum=0;
                for(int l=0; l<TOTAL_IC; l++){
                    //if(weight_array[i][l])
                    //    sum+=(ap_uint<13>)in_fmap_test[l][j][k];
                    //else
                    //    sum-=(ap_uint<13>)in_fmap_test[l][j][k];
                    sum+=(ap_uint<13>)in_fmap_test[l][j][k]*(ap_uint<13>)weight_array[i][l];
                }
                out_fmap_test[i][j][k]=convert(sum, threshold_array);
            }
        }
    }

    //pooling
    for(int i=0; i<TOTAL_OC; i++){
        for(int j=0; j<(HEIGHT/2); j++){
            for(int k=0; k<(WIDTH/2); k++){
                out_fmap_pool[i][j][k]=max(max(out_fmap_test[i][j*2][k*2],out_fmap_test[i][j*2][k*2+1]), max(out_fmap_test[i][j*2+1][k*2],out_fmap_test[i][j*2+1][k*2+1]));
            }
        }
    }


    // out fmap shift
    /*for(int i=0; i<TOTAL_OC; i++){
        if(i%5==0){
            //shift down
            for(int j=HEIGHT-1; j>=1; j--){
                for(int k=0; k<WIDTH; k++){
                    out_fmap_test[i][j][k]=out_fmap_test[i][j-1][k];
                }
            }
            for(int k=0; k<WIDTH; k++){
                out_fmap_test[i][0][k]=0;
            }
        }
        else if(i%5==1){
            //shift right
            for(int j=0; j<HEIGHT; j++){
                for(int k=WIDTH-1; k>=1; k--){
                    out_fmap_test[i][j][k]=out_fmap_test[i][j][k-1];
                }
                out_fmap_test[i][j][0]=0;
            }
        }
        else if(i%5==3){
            //shift left
            for(int j=0; j<HEIGHT; j++){
                for(int k=0; k<=WIDTH-2; k++){
                    out_fmap_test[i][j][k]=out_fmap_test[i][j][k+1];
                }
                out_fmap_test[i][j][WIDTH-1]=0;
            }
        }
        else if(i%5==4){
            //shift up
            for(int j=0; j<=HEIGHT-2; j++){
                for(int k=0; k<WIDTH; k++){
                    out_fmap_test[i][j][k]=out_fmap_test[i][j+1][k];
                }
            }
            for(int k=0; k<WIDTH; k++){
                out_fmap_test[i][HEIGHT-1][k]=0;
            }
        }
    }*/

    //out fmap shift with pool
    for(int i=0; i<TOTAL_OC; i++){
        if(i%5==0){
            //shift down
            for(int j=(HEIGHT/2)-1; j>=1; j--){
                for(int k=0; k<(WIDTH/2); k++){
                    out_fmap_pool[i][j][k]=out_fmap_pool[i][j-1][k];
                }
            }
            for(int k=0; k<(WIDTH/2); k++){
                out_fmap_pool[i][0][k]=0;
            }
        }
        else if(i%5==1){
            //shift right
            for(int j=0; j<(HEIGHT/2); j++){
                for(int k=(WIDTH/2)-1; k>=1; k--){
                    out_fmap_pool[i][j][k]=out_fmap_pool[i][j][k-1];
                }
                out_fmap_pool[i][j][0]=0;
            }
        }
        else if(i%5==3){
            //shift left
            for(int j=0; j<(HEIGHT/2); j++){
                for(int k=0; k<=(WIDTH/2)-2; k++){
                    out_fmap_pool[i][j][k]=out_fmap_pool[i][j][k+1];
                }
                out_fmap_pool[i][j][(WIDTH/2)-1]=0;
            }
        }
        else if(i%5==4){
            //shift up
            for(int j=0; j<=(HEIGHT/2)-2; j++){
                for(int k=0; k<(WIDTH/2); k++){
                    out_fmap_pool[i][j][k]=out_fmap_pool[i][j+1][k];
                }
            }
            for(int k=0; k<(WIDTH/2); k++){
                out_fmap_pool[i][(HEIGHT/2)-1][k]=0;
            }
        }
    }



    bool same=true;
    for(int i=0; i<TOTAL_OC; i++){
        for(int j=0; j<(HEIGHT/2); j++){
            for(int k=0; k<(WIDTH/2); k++){
                if(out_fmap_pool[i][j][k]!=out_fmap[i+j*TOTAL_OC].range(4*k+3,4*k)){
                    same=false;
                    cout<<"different in "<<i<<","<<j<<","<<k<<endl;
                    //break;
                }
            }
        }
    }
    /*bool same=true;
    for(int i=0; i<TOTAL_OC; i++){
        for(int j=0; j<HEIGHT; j++){
            for(int k=0; k<WIDTH; k++){
                if(out_fmap_test[i][j][k]!=out_fmap[i+j*TOTAL_OC].range(4*k+3,4*k)){
                    same=false;
                    cout<<"different in "<<i<<","<<j<<","<<k<<endl;
                    //break;
                }
            }
        }
    }*/
    cout<<"same:"<<same<<endl;
    cout<<TOTAL_IC<<endl;

    print_fmap_row2(out_fmap[21]);
    for(int i=0; i<(WIDTH/2); i++){
        cout<<out_fmap_pool[21][0][i]<<",";
    }
    cout<<endl;

    //cout<<endl<<"result:"<<endl;
    //for(int i=0; i<HEIGHT; i++){
    //    print_fmap_row(out_fmap[i]);
    //}
    return 0;
};
