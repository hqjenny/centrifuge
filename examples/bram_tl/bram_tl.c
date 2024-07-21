#include <stdio.h>
#include <stdint.h>
#define ACCEL_CONTROL 0x20000
#define AP_DONE_MASK 0b10

#define ACCEL_INT 0x20004
#define ACCEL_RETURN 0x20010
#define ACCEL_PARAM1 0x20018
#define ACCEL_PARAM2 0x20020
#define ACCEL_NIC 0x2002c
#define ACCEL_BRAM 0x22000

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
int top_accel(int param1, int param2) {
    // Disable interrupt for now
    reg_write32(ACCEL_INT, 0x0);
    
    // Set up pointer a and pointer b address
    reg_write32(ACCEL_PARAM1, (uint32_t)param1);
    reg_write32(ACCEL_PARAM2, (uint32_t)param2);
    //reg_write32(ACCEL_BRAM + param1, (uint32_t) 17);
    //reg_write32(ACCEL_BRAM + param2, (uint32_t) 18);
    
  //  reg_write32(ACCEL_BRAM, 0x1);
  //  reg_write32(ACCEL_BRAM+(param1 << 2), 0x7);
  //  reg_write32(ACCEL_BRAM+(param2 << 2), 0x8);

    uint64_t srcmac = reg_read64(ICENET_IO_BASE+ICENET_MACADDR);
    printf("srcmac %d\n", srcmac);
    reg_write32(ACCEL_CONTROL+0x4000, srcmac);
    reg_write32(ACCEL_CONTROL+0x4004, srcmac >> 32);
    reg_write32(ACCEL_CONTROL+0x400c, srcmac);
    reg_write32(ACCEL_CONTROL+0x4010, srcmac >> 32);

    // Write to ap_start to start the execution 
    reg_write32(ACCEL_CONTROL, 0x1);
    //printf("Accel Control: %x\n", reg_read32(ACCEL_CONTROL));
    
    // Done?
    int done = 0;
    while (!done){
        done = reg_read32(ACCEL_CONTROL) & AP_DONE_MASK;
    }

    //int bram = reg_read32(ACCEL_BRAM);
    //printf("bram = %d\n", bram);
    //int ret = reg_read32(ACCEL_BRAM+param1);
    int ret = reg_read32(ACCEL_RETURN);
    printf("ret = %d\n", ret);
    return ret;
}
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

int main(){

#ifdef CUSTOM_DRIVER
    top_accel(4,12);
#endif
    printf("main\n");
   return 0;
}

