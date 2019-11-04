#include "/home/centos/hls-fs/hls/sw/bm//mmio.h"
#define ACCEL_BASE 0x20000
#define AP_DONE_MASK 0b10
#define ACCEL_INT 0x4
void top_wrapper() {
    // Disable Interrupt
    reg_write32(ACCEL_BASE + ACCEL_INT, 0x0);

    // Write to ap_start to start the execution 
    reg_write32(ACCEL_BASE, 0x1);

    // Done?
    int done = 0;
    while (!done){
        done = reg_read32(ACCEL_BASE) & AP_DONE_MASK;
    }
}
