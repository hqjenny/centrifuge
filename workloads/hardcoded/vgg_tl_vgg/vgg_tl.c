//#ifdef CUSTOM_DRIVER
#include "bm_wrapper.h"
//#endif
#include "time.h"

#define IFMAP_DW 8
#define FMAP_DW 4
#define WEIGHT_DW 8
#define OUT_DW 8
#define PE 1
#define F 3
#define PAD 1


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

#define FACTOR (OUT_DW/FMAP_DW)


static char fmap [DIM_LAYER0*DIM_LAYER0*CH_LAYER0];
static char k1[F * F * CH_LAYER0 * CH_LAYER1/PE]; 
static char k2[F * F * CH_LAYER1 * CH_LAYER1/PE]; 
static char k3[F * F * CH_LAYER1 * CH_LAYER2/PE]; 
static char k4[F * F * CH_LAYER2 * CH_LAYER2/PE]; 
static char k5[F * F * CH_LAYER2 * CH_LAYER3/PE]; 
static char k6[F * F * CH_LAYER3 * CH_LAYER3/PE]; 
static char k7[F * F * CH_LAYER3 * CH_LAYER3/PE]; 
static char k8[F * F * CH_LAYER3 * CH_LAYER4/PE]; 
static char k9[F * F * CH_LAYER4 * CH_LAYER4/PE]; 
static char k10[F * F * CH_LAYER4 * CH_LAYER4/PE]; 
static char k11[F * F * CH_LAYER4 * CH_LAYER5/PE]; 
static char k12[F * F * CH_LAYER5 * CH_LAYER5/PE]; 
static char k13[F * F * CH_LAYER5 * CH_LAYER5/PE]; 
static char k14[FC_DIM0 * FC_DIM0 *CH_LAYER5 * FC_LAYER1/PE]; 
static char k15[FC_LAYER1 * FC_LAYER1/PE]; 
static char k16[FC_LAYER1 * FC_LAYER2/PE]; 
static char out [FC_LAYER2/FACTOR];

main (){
  int rep = 1;
  uint64_t begin, end, dur;
  begin = read_cycle();
//  #ifdef CUSTOM_DRIVER
  //vgg_wrapper( fmap,  k1,  k2,  k3,  k4,  k5,  k6,  k7,  k8,  k9,  k10,  k11,  k12,  k13,  k14,  k15,  k16,  out, rep);
  vgg0_wrapper( fmap, out, rep);
//  #else
//  #endif 
  end = read_cycle();
  duration(begin, end);
}
