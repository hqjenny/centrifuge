#pragma once
namespace TB{
#include "ap_int.h"
#include "param.h"
  const ap_uint<WEIGHT_DW*PE> k1[F][F][CH_LAYER0][CH_LAYER1/PE]; 
  const ap_uint<WEIGHT_DW*PE> k2[F][F][CH_LAYER1][CH_LAYER1/PE]; 
  const ap_uint<WEIGHT_DW*PE> k3[F][F][CH_LAYER1][CH_LAYER2/PE]; 
  const ap_uint<WEIGHT_DW*PE> k4[F][F][CH_LAYER2][CH_LAYER2/PE]; 
  const ap_uint<WEIGHT_DW*PE> k5[F][F][CH_LAYER2][CH_LAYER3/PE]; 
  const ap_uint<WEIGHT_DW*PE> k6[F][F][CH_LAYER3][CH_LAYER3/PE]; 
  const ap_uint<WEIGHT_DW*PE> k7[F][F][CH_LAYER3][CH_LAYER3/PE]; 
  const ap_uint<WEIGHT_DW*PE> k8[F][F][CH_LAYER3][CH_LAYER4/PE]; 
  const ap_uint<WEIGHT_DW*PE> k9[F][F][CH_LAYER4][CH_LAYER4/PE]; 
  const ap_uint<WEIGHT_DW*PE> k10[F][F][CH_LAYER4][CH_LAYER4/PE]; 
  const ap_uint<WEIGHT_DW*PE> k11[F][F][CH_LAYER4][CH_LAYER5/PE]; 
  const ap_uint<WEIGHT_DW*PE> k12[F][F][CH_LAYER5][CH_LAYER5/PE]; 
  const ap_uint<WEIGHT_DW*PE> k13[F][F][CH_LAYER5][CH_LAYER5/PE]; 
  const ap_uint<WEIGHT_DW*PE> k14[FC_DIM0 * FC_DIM0 *CH_LAYER5][FC_LAYER1/PE]; 
  const ap_uint<WEIGHT_DW*PE> k15[FC_LAYER1][FC_LAYER1/PE]; 
  const ap_uint<WEIGHT_DW*PE> k16[FC_LAYER1][FC_LAYER2/PE]; 

	const int rep = REP;
  const ap_uint<IFMAP_DW> fmap[rep * DIM_LAYER0*DIM_LAYER0*CH_LAYER0];
  ap_uint<OUT_DW> out[rep * FC_LAYER2*FMAP_DW/OUT_DW];
}
