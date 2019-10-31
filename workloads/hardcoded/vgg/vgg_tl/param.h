#pragma once
#include "ap_int.h"

#define IFMAP_DW 8
#define FMAP_DW 4
#define WEIGHT_DW 8
#define OUT_DW 8
#define PE 1
#define F 3
#define PAD 1
#define REP 1

typedef ap_uint<2> T_W;
typedef ap_uint<16> T_SUM;

#define DIM_LAYER0 64
#define CH_LAYER0 3

#define DIM_LAYER1 DIM_LAYER0
#define CH_LAYER1 8

#define DIM_LAYER2 (DIM_LAYER1/2)
#define CH_LAYER2 (CH_LAYER1 * 2) 

#define DIM_LAYER3 (DIM_LAYER2/2)
#define CH_LAYER3 (CH_LAYER2 * 2) 

#define DIM_LAYER4 (DIM_LAYER3/2)
#define CH_LAYER4 (CH_LAYER3*2)

#define DIM_LAYER5 (DIM_LAYER4/2)
#define CH_LAYER5 CH_LAYER4

#define FC_DIM0 (DIM_LAYER5/2)
#define FC_LAYER1 (CH_LAYER4*8)
#define FC_LAYER2 (CH_LAYER4*2) 
