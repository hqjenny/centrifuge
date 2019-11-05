#include "/home/centos/firesim/hls/sw/bm//mmio.h"
#define ACCEL_BASE 0x20000
#define AP_DONE_MASK 0b10
#define ACCEL_INT 0x4
#define ACCEL_p_aAddr_m_Val_0 0x10
#define ACCEL_p_aAddr_m_Val_1 0x14
#define ACCEL_p_bAddr_m_Val_0 0x1c
#define ACCEL_p_bAddr_m_Val_1 0x20
#define ACCEL_p_cAddr_m_Val_0 0x28
#define ACCEL_p_cAddr_m_Val_1 0x2c
#define ACCEL_p_xAddr_m_Val_0 0x34
#define ACCEL_p_xAddr_m_Val_1 0x38
#define ACCEL_p_aColBlocks_0 0x40
#define ACCEL_p_aRowBlocks_0 0x48
#define ACCEL_p_bColBlocks_0 0x50
#define ACCEL_p_aLd_0 0x58
#define ACCEL_p_bLd_0 0x60
#define ACCEL_p_cLd_0 0x68
#define ACCEL_p_xLd_0 0x70
#define ACCEL_p_transpBlocks_0 0x78
#define ACCEL_p_postScale_0 0x80
void gemx_wrapper(uint64_t p_aAddr_m_Val, uint64_t p_bAddr_m_Val, uint64_t p_cAddr_m_Val, uint64_t p_xAddr_m_Val, uint32_t p_aColBlocks, uint32_t p_aRowBlocks, uint32_t p_bColBlocks, uint32_t p_aLd, uint32_t p_bLd, uint32_t p_cLd, uint32_t p_xLd, uint32_t p_transpBlocks, uint32_t p_postScale) {
    // Disable Interrupt
    reg_write32(ACCEL_BASE + ACCEL_INT, 0x0);
    reg_write32(ACCEL_BASE + ACCEL_p_aAddr_m_Val_0, (uint32_t) (p_aAddr_m_Val));
    reg_write32(ACCEL_BASE + ACCEL_p_aAddr_m_Val_1, (uint32_t) (p_aAddr_m_Val >> 32));
    reg_write32(ACCEL_BASE + ACCEL_p_bAddr_m_Val_0, (uint32_t) (p_bAddr_m_Val));
    reg_write32(ACCEL_BASE + ACCEL_p_bAddr_m_Val_1, (uint32_t) (p_bAddr_m_Val >> 32));
    reg_write32(ACCEL_BASE + ACCEL_p_cAddr_m_Val_0, (uint32_t) (p_cAddr_m_Val));
    reg_write32(ACCEL_BASE + ACCEL_p_cAddr_m_Val_1, (uint32_t) (p_cAddr_m_Val >> 32));
    reg_write32(ACCEL_BASE + ACCEL_p_xAddr_m_Val_0, (uint32_t) (p_xAddr_m_Val));
    reg_write32(ACCEL_BASE + ACCEL_p_xAddr_m_Val_1, (uint32_t) (p_xAddr_m_Val >> 32));
    reg_write32(ACCEL_BASE + ACCEL_p_aColBlocks_0, (uint32_t) (p_aColBlocks));
    reg_write32(ACCEL_BASE + ACCEL_p_aRowBlocks_0, (uint32_t) (p_aRowBlocks));
    reg_write32(ACCEL_BASE + ACCEL_p_bColBlocks_0, (uint32_t) (p_bColBlocks));
    reg_write32(ACCEL_BASE + ACCEL_p_aLd_0, (uint32_t) (p_aLd));
    reg_write32(ACCEL_BASE + ACCEL_p_bLd_0, (uint32_t) (p_bLd));
    reg_write32(ACCEL_BASE + ACCEL_p_cLd_0, (uint32_t) (p_cLd));
    reg_write32(ACCEL_BASE + ACCEL_p_xLd_0, (uint32_t) (p_xLd));
    reg_write32(ACCEL_BASE + ACCEL_p_transpBlocks_0, (uint32_t) (p_transpBlocks));
    reg_write32(ACCEL_BASE + ACCEL_p_postScale_0, (uint32_t) (p_postScale));

    // Write to ap_start to start the execution 
    reg_write32(ACCEL_BASE, 0x1);

    // Done?
    int done = 0;
    while (!done){
        done = reg_read32(ACCEL_BASE) & AP_DONE_MASK;
    }
}
