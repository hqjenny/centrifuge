#ifndef GEMX_TL_H
#define GEMX_TL_H

#pragma once

//#include "cblas_wrapper.hpp"
//namespace BCL {
//namespace experimental {
//
////template <typename short>
//void gemx_accel(const CBLAS_LAYOUT layout,
//        const CBLAS_TRANSPOSE transa, const CBLAS_TRANSPOSE transb,
//        const int m, const int n, const int k,
//        const short alpha, const short* a, const int lda,
//        const short* b, const int ldb, const short beta,
//        short* c, const int ldc);
//}
//}

#include <stdlib.h> 
#include <stdio.h> 
#include <assert.h> 
#include "os_utils.h"
#include "gemx_params.h"
#include "gemx_tl.hpp"
#include "cblas_wrapper.hpp"
#define ACCEL_CONTROL 0x20000
#define ACCEL_BASE 0x20000
#define AP_DONE_MASK 0b10

#define ACCEL_INT 0x20004

namespace BCL {
  namespace experimental {


void  gemx_accel_wrapper(
    char *p_aAddr,
    char *p_bAddr,
    char *p_cAddr,
    char *p_xAddr,
    unsigned int p_aColBlocks,
    unsigned int p_aRowBlocks,
    unsigned int p_bColBlocks,
    unsigned int p_aLd,
    unsigned int p_bLd,
    unsigned int p_cLd,
    unsigned int p_xLd,
    unsigned int p_transpBlocks,
    int32_t p_postScale
    ) {

  uint64_t addr;

  // Disable interrupt for now
  access_addr(ACCEL_INT, OUT, 0);

  // Set up pointer addresses
  addr = vtop_translate(p_aAddr);
  access_addr(ACCEL_BASE + 0x10, OUT, (uint32_t)addr);
  access_addr(ACCEL_BASE + 0x10 + 0x4, OUT, (uint32_t)addr >> 32);

  addr = vtop_translate(p_bAddr);
  access_addr(ACCEL_BASE + 0x1c, OUT, (uint32_t)addr);
  access_addr(ACCEL_BASE + 0x1c + 0x4, OUT, (uint32_t)addr >> 32);

  addr = vtop_translate(p_cAddr);
  access_addr(ACCEL_BASE + 0x28, OUT, (uint32_t)addr);
  access_addr(ACCEL_BASE + 0x28 + 0x4, OUT, (uint32_t)addr >> 32);

  addr = vtop_translate(p_xAddr);
  access_addr(ACCEL_BASE + 0x34, OUT, (uint32_t)addr);
  access_addr(ACCEL_BASE + 0x34 + 0x4, OUT, (uint32_t)addr >> 32);

  access_addr(ACCEL_BASE + 0x40, OUT, p_aColBlocks);
  access_addr(ACCEL_BASE + 0x48, OUT, p_aRowBlocks);
  access_addr(ACCEL_BASE + 0x50, OUT, p_bColBlocks);
  access_addr(ACCEL_BASE + 0x58, OUT, p_aLd);
  access_addr(ACCEL_BASE + 0x60, OUT, p_bLd);
  access_addr(ACCEL_BASE + 0x68, OUT, p_cLd);
  access_addr(ACCEL_BASE + 0x70, OUT, p_xLd);
  access_addr(ACCEL_BASE + 0x78, OUT, p_transpBlocks);
  access_addr(ACCEL_BASE + 0x80, OUT, p_postScale);

  // Write to ap_start to start the execution 
  access_addr(ACCEL_CONTROL, OUT, 0x1);
  //printf("Accel Control: %x\n", reg_read32(ACCEL_CONTROL));

  // Done?
  int done = 0;
  while (!done){
    done = access_addr(ACCEL_CONTROL, IN, 0x1) & AP_DONE_MASK;
  }
}

void gemx_accel(const CBLAS_LAYOUT layout,
        const CBLAS_TRANSPOSE transa, const CBLAS_TRANSPOSE transb,
        const int m, const int n, const int k,
        const short alpha, const short* a, const int lda,
        const short* b, const int ldb, const short beta,
        short* c, const int ldc) {

  // We don't currently support other params in the accelerator
  assert(alpha == 1 && beta == 1);
  assert(m >= 256 && n >= 256 && k >= 256);

  unsigned int t_DdrWidth = GEMX_ddrWidth; 
  unsigned int t_XDdrWidth = GEMX_XddrWidth;
  unsigned int t_aColMemWords = GEMX_gemmKBlocks;  
  unsigned int t_aRowMemWords = GEMX_gemmMBlocks;  
  unsigned int t_bColMemWords = GEMX_gemmNBlocks; 
  const unsigned int l_aColBlocks = k / (t_DdrWidth * t_aColMemWords);
  const unsigned int l_aRowBlocks = m / (t_DdrWidth * t_aRowMemWords);
  const unsigned int l_bColBlocks = n / (t_DdrWidth * t_bColMemWords);
  const unsigned int l_aLd  = lda / t_DdrWidth;
  const unsigned int l_bLd  = ldb / t_DdrWidth;
  const unsigned int l_cLd  = ldc / t_DdrWidth;
  const unsigned int l_xLd 	= ldc / t_XDdrWidth;
  const int32_t l_postScale = 0; // This parameter is not used
  unsigned int l_transpBlocks = l_aColBlocks * l_aRowBlocks * l_bColBlocks *t_aRowMemWords;

  gemx_accel_wrapper( (char*)a, (char*)b, (char*)c, (char*)c, l_aColBlocks, l_aRowBlocks, l_bColBlocks, l_aLd, l_bLd, l_cLd, l_xLd, l_transpBlocks, l_postScale);

}
}
}



#endif
