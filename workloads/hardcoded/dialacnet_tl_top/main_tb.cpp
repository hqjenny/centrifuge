#include <iostream>
#include <cstdlib>
#include <fstream>
#include "para.h"
using namespace std;
extern "C"
void shiftshuffle(ap_uint<FM_W*PA_0>* fmap, ap_uint<FM_W*PA_0> * out,
		ap_uint<W_W * PE_0 * PA_0> *k0,
		ap_uint<W_W * PE_0 * PA_0> *k1,
		ap_uint<W_W * PA_0 * PA_0> *k2,
		int FM_D, 
		int FM_CH,
		int th_i,
		bool pool,
		int rep);


int main(){
	// TEST 1
	static ap_uint<FM_W*PA_0> fmap[REP*TEST_D*TEST_D*TEST_C/PA_0];
	for(int i=0;i<TEST_D*TEST_D*TEST_C/PA_0;i++)
		fmap[i] = rand();
	static ap_uint<FM_W*PA_0> test1[REP*TEST_D*TEST_D*TEST_C/PE_0];
	ap_uint<W_W * PE_0 * PA_0> k0[TEST_C*TEST_C/PE_0/PA_0];
	for(int i=0;i<TEST_C*TEST_C/PA_0/PE_0;i++)
		k0[i] = rand();
	ap_uint<W_W * PE_0 * PA_0> k1[TEST_C*TEST_C/PE_0/PA_0];
	for(int i=0;i<TEST_C*TEST_C/PA_0/PE_0;i++)
		k1[i] = rand();
	ap_uint<W_W * PA_0 * PA_0> k2[TEST_C*TEST_C/PE_0/PA_0];
	for(int i=0;i<TEST_C*TEST_C/PA_0/PE_0;i++)
		k2[i] = rand();
	shiftshuffle(fmap, test1, k0,k1,k2, TEST_D, TEST_C, 0, false, REP);
	ofstream os;
	os.open("test1.txt");
	for(int i=0;i<REP*TEST_C*TEST_D*TEST_D/PE_0;i++)
		os<<test1[i]<<',';
	os.close();
	/*
	// TEST 2
	ap_uint<W_W * PE_0 * PA_0> k01[TEST_C*TEST_C/PE_0/PA_0*2];
	for(int i=0;i<TEST_C*TEST_C/PA_0/PE_0*2;i++)
		k01[i] = rand();
	ap_uint<W_W * PE_0 * PA_0> k11[TEST_C*TEST_C/PE_0/PA_0*4];
	for(int i=0;i<TEST_C*TEST_C/PA_0/PE_0*4;i++)
		k11[i] = rand();
	ap_uint<W_W * PE_0 * PA_0> k21[TEST_C*TEST_C/PE_0/PA_0*2];
	for(int i=0;i<TEST_C*TEST_C/PA_0/PE_0*2;i++)
		k21[i] = rand();
	static ap_uint<FM_W*PA_0> test2[TEST_D*TEST_D*TEST_C/PE_0/2];
	shiftshuffle(fmap, test2, k0,k1,k2, TEST_D, TEST_C, 1, true, REP);
	os.open("test2.txt");
	for(int i=0;i<REP*TEST_C*TEST_D*TEST_D/PE_0/2;i++)
		os<<test2[i]<<',';
	os.close();
	*/
	return 0;
}
