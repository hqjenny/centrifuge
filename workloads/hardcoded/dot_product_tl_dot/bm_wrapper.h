#include "/home/centos/hls-fs/hls/sw/bm//mmio.h"
#define ACCEL_BASE 0x20000
#define AP_DONE_MASK 0b10
#define ACCEL_INT 0x4
#define ACCEL_ap_return_0 0x10
#define ACCEL_a_0 0x18
#define ACCEL_a_1 0x1c
#define ACCEL_b_0 0x24
#define ACCEL_b_1 0x28
#define ACCEL_length_r_0 0x30
uint32_t dot_wrapper(uint64_t a, uint64_t b, uint32_t length_r) {
    // Disable Interrupt
    reg_write32(ACCEL_BASE + ACCEL_INT, 0x0);
    reg_write32(ACCEL_BASE + ACCEL_a_0, (uint32_t) (a));
    reg_write32(ACCEL_BASE + ACCEL_a_1, (uint32_t) (a >> 32));
    reg_write32(ACCEL_BASE + ACCEL_b_0, (uint32_t) (b));
    reg_write32(ACCEL_BASE + ACCEL_b_1, (uint32_t) (b >> 32));
    reg_write32(ACCEL_BASE + ACCEL_length_r_0, (uint32_t) (length_r));

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
