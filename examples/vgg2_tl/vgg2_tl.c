//#ifdef CUSTOM_DRIVER
#include "bm_wrapper.h"
//#endif
#include "time.h"

#define ICENET_NAME "icenet"
#define ICENET_IO_BASE  0x10016000
#define ICENET_SEND_REQ 0
#define ICENET_RECV_REQ 8
#define ICENET_SEND_COMP 16
#define ICENET_RECV_COMP 18
#define ICENET_COUNTS 20
#define ICENET_MACADDR 24
#define ICENET_IO_SIZE 32

#define CIRC_BUF_LEN 16
#define ALIGN_BYTES 8
#define ALIGN_MASK 0x7
#define ALIGN_SHIFT 3

size_t sendq_max;
size_t recvq_max;
uint64_t mac;

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

static inline int send_req_avail(uint64_t* nic)
{
  return nic[ICENET_COUNTS] & 0xf;
}

static inline int recv_req_avail(uint64_t* nic)
{
  return (nic[ICENET_COUNTS] >> 4) & 0xf;
}

static inline int send_comp_avail(uint64_t* nic)
{
  return (nic[ICENET_COUNTS] >> 8) & 0xf;
}

static inline int recv_comp_avail(uint64_t* nic)
{
  return (nic[ICENET_COUNTS] >> 12) & 0xf;
}


void ice_post_send(uint64_t* nic, int last, uintptr_t paddr, size_t len)
{
  uint64_t command = 0;

  if( ((paddr & 0x7ll) != 0) ||
      ((len % 8) != 0)) {
    printf("paddr: 0x%lx, len = 0x%lx\n", paddr, len);
  }
  command = (len << 48) | (paddr & 0xffffffffffffL);
  command |= last ? 0 : (1ul << 63);

  /* iowrite64(command, nic->iomem + ICENET_SEND_REQ); */
  //writeq(command, nic->iomem + ICENET_SEND_REQ);
  nic[ICENET_SEND_REQ] = command;
}

void ice_post_recv(uint64_t* nic, uintptr_t paddr)
{
  if((paddr & 0x7) != 0) {
    //panic("Unaligned receive buffer: %lx\n", paddr);
    ;
  }
  /* iowrite64(paddr, nic->iomem + ICENET_RECV_REQ); */
  //writeq(paddr, nic->iomem + ICENET_RECV_REQ);
  nic[ICENET_RECV_REQ] = paddr;
}

void ice_drain_sendq(uint64_t* nic)
{
  /* Poll until there are no more pending sends */
  while(send_req_avail(nic) < sendq_max) { 
    ;
  }

  /* Drain send_compq */
  while (send_comp_avail(nic) > 0) {
    //ioread16(nic->iomem + ICENET_SEND_COMP);
    uint64_t tmp = nic[ICENET_SEND_COMP];
  }

  return;
}

size_t ice_recv_one(uint64_t* nic)
{
  /* Wait for there to be something in the recv_comp Q */
  while(recv_comp_avail(nic) == 0) { ; }

  /* Pop exactly one thing off Q */
  //return (size_t)ioread16(nic->iomem + ICENET_RECV_COMP);
  return (size_t)nic[ICENET_RECV_COMP];
}



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
  uint64_t srcmac = reg_read64(ICENET_IO_BASE+ICENET_MACADDR);
  uint64_t dstmac = srcmac;
  printf("srcmac %d\n", srcmac);

  int rep = 1;
  uint64_t begin, end, dur;
  begin = read_cycle();
//  #ifdef CUSTOM_DRIVER
//  vgg_wrapper( fmap,  k1,  k2,  k3,  k4,  k5,  k6,  k7,  k8,  k9,  k10,  k11,  k12,  k13,  k14,  k15,  k16,  out, rep);
    vgg_wrapper(fmap, out, rep, srcmac, dstmac);
//  #else
//  #endif 
  end = read_cycle();
  duration(begin, end);
}
