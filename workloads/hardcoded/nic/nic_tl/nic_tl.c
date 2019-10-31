#include <stdio.h>
#include <stdint.h>
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
#ifdef CUSTOM_DRIVER
#include "/scratch/qijing.huang/firesim_new/hls/sw/bm//mmio.h"
#include "bm_wrapper.h"
#endif

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

#define COUNT 0
int main(){
    int compute_cycles = 400;
    int load_count = 16;
    int send_count = COUNT;
    int recv_count = COUNT;
    int packet_size = 1;

//int main(int argc,char* argv[]) {
//
//  printf("argc %d\n", argc);
//  if(argc!=6) {
//    printf("Please specify the right number of args!\n");
//    return 0;
//  } 
//
//  int compute_cycles = argv[1];
//  int load_count = argv[2];
//  int send_count = argv[3];
//  int recv_count = argv[4];
//  int packet_size = argv[5];
//
  uint64_t begin, end, dur;

#ifdef CUSTOM_DRIVER
  //top_accel(4,12);
  uint64_t mem_V[1024];

  uint64_t srcmac = reg_read64(ICENET_IO_BASE+ICENET_MACADDR);
  uint64_t dstmac = srcmac;
  printf("srcmac %d\n", srcmac);

  begin = read_cycle();
  int ret = top_wrapper(compute_cycles, load_count, send_count, recv_count, packet_size, mem_V, srcmac, dstmac);
  end = read_cycle();
  printf("ret: %d\n", ret);
#endif
  duration(begin, end);

  return 0;
}

