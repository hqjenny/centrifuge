#include "/scratch/qijing.huang/firesim/hls/sw/bm//mmio.h"
#define ACCEL_BASE 0x20000
#define AP_DONE_MASK 0b10
#define ACCEL_INT 0x4
#define ACCEL_fmap_V_0 0x10
#define ACCEL_fmap_V_1 0x14
#define ACCEL_out_V_0 0x1c
#define ACCEL_out_V_1 0x20
#define ACCEL_rep_0 0x28
#define ACCEL_srcmac_V_0 0x30
#define ACCEL_srcmac_V_1 0x34
#define ACCEL_dstmac_V_0 0x3c
#define ACCEL_dstmac_V_1 0x40
void vgg_wrapper(uint64_t fmap_V, uint64_t out_V, uint32_t rep, uint64_t srcmac_V, uint64_t dstmac_V) {
    // Disable Interrupt
    reg_write32(ACCEL_BASE + ACCEL_INT, 0x0);
    reg_write32(ACCEL_BASE + ACCEL_fmap_V_0, (uint32_t) (fmap_V));
    reg_write32(ACCEL_BASE + ACCEL_fmap_V_1, (uint32_t) (fmap_V >> 32));
    reg_write32(ACCEL_BASE + ACCEL_out_V_0, (uint32_t) (out_V));
    reg_write32(ACCEL_BASE + ACCEL_out_V_1, (uint32_t) (out_V >> 32));
    reg_write32(ACCEL_BASE + ACCEL_rep_0, (uint32_t) (rep));
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
}
