#include "/scratch/qijing.huang/firesim_new/hls/sw/bm//mmio.h"
#define ACCEL_BASE 0x20000
#define AP_DONE_MASK 0b10
#define ACCEL_INT 0x4
#define ACCEL_ap_return_0 0x10
#define ACCEL_compute_cycles_0 0x18
#define ACCEL_load_count_0 0x20
#define ACCEL_send_count_0 0x28
#define ACCEL_recv_count_0 0x30
#define ACCEL_packet_size_0 0x38
#define ACCEL_mem_V_0 0x40
#define ACCEL_mem_V_1 0x44
#define ACCEL_srcmac_V_0 0x4c
#define ACCEL_srcmac_V_1 0x50
#define ACCEL_dstmac_V_0 0x58
#define ACCEL_dstmac_V_1 0x5c
uint32_t top_wrapper(uint32_t compute_cycles, uint32_t load_count, uint32_t send_count, uint32_t recv_count, uint32_t packet_size, uint64_t mem_V, uint64_t srcmac_V, uint64_t dstmac_V) {
    // Disable Interrupt
    reg_write32(ACCEL_BASE + ACCEL_INT, 0x0);
    reg_write32(ACCEL_BASE + ACCEL_compute_cycles_0, (uint32_t) (compute_cycles));
    reg_write32(ACCEL_BASE + ACCEL_load_count_0, (uint32_t) (load_count));
    reg_write32(ACCEL_BASE + ACCEL_send_count_0, (uint32_t) (send_count));
    reg_write32(ACCEL_BASE + ACCEL_recv_count_0, (uint32_t) (recv_count));
    reg_write32(ACCEL_BASE + ACCEL_packet_size_0, (uint32_t) (packet_size));
    reg_write32(ACCEL_BASE + ACCEL_mem_V_0, (uint32_t) (mem_V));
    reg_write32(ACCEL_BASE + ACCEL_mem_V_1, (uint32_t) (mem_V >> 32));
    reg_write32(ACCEL_BASE + ACCEL_srcmac_V_0, (uint32_t) (srcmac_V));
    reg_write32(ACCEL_BASE + ACCEL_srcmac_V_1, (uint32_t) (srcmac_V >> 32));
    reg_write32(ACCEL_BASE + ACCEL_dstmac_V_0, (uint32_t) (dstmac_V));
    reg_write32(ACCEL_BASE + ACCEL_dstmac_V_1, (uint32_t) (dstmac_V >> 32));

    // Write to ap_start to start the execution 
    reg_write32(ACCEL_BASE, 0x1);

    // Done?
    int done = 0;
    while (!done){
        done = reg_read32(ACCEL_BASE) & AP_DONE_MASK;
    }

    uint32_t ret_val = 0;
    ret_val = (reg_read32(ACCEL_BASE + ACCEL_ap_return_0)) | ret_val;
    return ret_val;
}
