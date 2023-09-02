#include <stdio.h>
#include <stdint.h>
#include "ap_int.h"
#include "hls_stream.h"
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
    
    reg_write32(ACCEL_BRAM, 0x1);
    reg_write32(ACCEL_BRAM+(param1 << 2), 0x7);
    reg_write32(ACCEL_BRAM+(param2 << 2), 0x8);

    // Write to ap_start to start the execution 
    reg_write32(ACCEL_CONTROL, 0x1);
    //printf("Accel Control: %x\n", reg_read32(ACCEL_CONTROL));
    
    // Done?
    int done = 0;
    while (!done){
        done = reg_read32(ACCEL_CONTROL) & AP_DONE_MASK;
    }


    int bram = reg_read32(ACCEL_BRAM);
    printf("bram = %d\n", bram);
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

 //   val ethType = UInt(ETH_TYPE_BITS.W)
 //   val srcmac  = UInt(ETH_MAC_BITS.W)
 //   val dstmac  = UInt(ETH_MAC_BITS.W)
 //   val padding = UInt(ETH_PAD_BITS.W)
 //   val NET_IF_WIDTH = 64
 //   val NET_IF_BYTES = NET_IF_WIDTH/8
 //   val NET_LEN_BITS = 16
 // 
 //   val ETH_MAX_BYTES = 1520
 //   val ETH_HEAD_BYTES = 16
 //   val ETH_MAC_BITS = 48
 //   val ETH_TYPE_BITS = 16
 //   val ETH_PAD_BITS = 16

int top(int param1, int param2, uint64_t* nic, int paramtable[1600], hls::stream<ap_uint<128> >& req_head, hls::stream<ap_uint<65> >& req_data, hls::stream<ap_uint<128> >& resp_head, hls::stream<ap_uint<65> >& resp_data, ap_uint<64>srcmac, ap_uint<64>dstmac) {
//#pragma HLS dataflow
#pragma HLS INTERFACE s_axilite port=nic bundle=control
#pragma HLS INTERFACE m_axi port=nic offset=slave bundle=gmem0
#pragma HLS interface ap_fifo port=req_head
#pragma HLS interface ap_fifo port=req_data
#pragma HLS interface ap_fifo port=resp_head
#pragma HLS interface ap_fifo port=resp_data

#pragma HLS interface s_axilite bundle=control port=param1
#pragma HLS interface s_axilite bundle=control port=param2
#pragma HLS interface s_axilite bundle=control port=paramtable
#pragma HLS interface s_axilite bundle=control port=srcmac
#pragma HLS interface s_axilite bundle=control port=dstmac
#pragma HLS interface s_axilite bundle=control port=return

  ap_uint<16> RMEM_REQ_ETH_TYPE = 0x0408L;
  ap_uint<16> RMEM_RESP_ETH_TYPE = 0x0508L;

  ap_uint<128> resp_eth_head;
  ap_uint<128> req_eth_head;
  
  resp_eth_head.range(15,0) = 0;
  resp_eth_head.range(63,16) = dstmac;
  resp_eth_head.range(111,64) = srcmac;
  resp_eth_head.range(127,112) = RMEM_RESP_ETH_TYPE; 
  ap_uint<65> send_data;
  send_data(63,0) = 77;
  send_data(64,64) = 1;
  
  // Send a request
  volatile int count = 1;
  if (count == 1) 
    resp_head.write(resp_eth_head);
  count ++;
  if (count == 2) 
  resp_data.write(send_data);
  count ++;

  // Loopback

  ap_uint<64> read_head;
  ap_uint<65> recv_data;
  if (count == 3) 
   read_head = req_head.read();
  count ++;
  if (count == 4) 
   recv_data = req_data.read();
  count ++;
  //recvq_max = recv_req_avail(nic);
  //sendq_max = send_req_avail(nic);
	//int sum = paramtable[param1] + paramtable[param2];
  //printf("recvq_max %d\n", recvq_max);
  //printf("sendq_max %d\n", sendq_max);
	return recv_data.range(63,0) + recv_data.range(64,64);
}

int main(){

#ifdef CUSTOM_DRIVER
    top_accel(4,12);
#endif
    printf("main\n");
   return 0;
}

