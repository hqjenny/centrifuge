#include <stdlib.h> 
#include <stdio.h> 
#include "mmio.h"
#include "time.h"
#include "gemx_params.h"
#define ACCEL_CONTROL 0x20000
#define ACCEL_BASE 0x20000
#define AP_DONE_MASK 0b10

#define ACCEL_INT 0x20004

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
  // Disable interrupt for now
  reg_write32(ACCEL_INT, 0x0);

  // Set up pointer addresses
  reg_write32(ACCEL_BASE + 0x10, (uint32_t)p_aAddr);
  reg_write32(ACCEL_BASE + 0x1c, (uint32_t)p_bAddr);
  reg_write32(ACCEL_BASE + 0x28, (uint32_t)p_cAddr);
  reg_write32(ACCEL_BASE + 0x34, (uint32_t)p_xAddr);
  reg_write32(ACCEL_BASE + 0x40, (uint32_t)p_aColBlocks);
  reg_write32(ACCEL_BASE + 0x48, (uint32_t)p_aRowBlocks);
  reg_write32(ACCEL_BASE + 0x50, (uint32_t)p_bColBlocks);
  reg_write32(ACCEL_BASE + 0x58, (uint32_t)p_aLd);
  reg_write32(ACCEL_BASE + 0x60, (uint32_t)p_bLd);
  reg_write32(ACCEL_BASE + 0x68, (uint32_t)p_cLd);
  reg_write32(ACCEL_BASE + 0x70, (uint32_t)p_xLd);
  reg_write32(ACCEL_BASE + 0x78, (uint32_t)p_transpBlocks);
  reg_write32(ACCEL_BASE + 0x80, (uint32_t)p_postScale);

  // Write to ap_start to start the execution 
  reg_write32(ACCEL_CONTROL, 0x1);
  //printf("Accel Control: %x\n", reg_read32(ACCEL_CONTROL));

  // Done?
  int done = 0;
  while (!done){
    done = reg_read32(ACCEL_CONTROL) & AP_DONE_MASK;
  }
}

void gemx_accel(short* C, short * A, short* B, int m, int n, int p){

  unsigned int t_DdrWidth = GEMX_ddrWidth; 
  unsigned int t_XDdrWidth = GEMX_XddrWidth;
  unsigned int t_aColMemWords = GEMX_gemmKBlocks;  
  unsigned int t_aRowMemWords = GEMX_gemmMBlocks;  
  unsigned int t_bColMemWords = GEMX_gemmNBlocks; 
  const unsigned int l_aColBlocks = n / (t_DdrWidth * t_aColMemWords);
  const unsigned int l_aRowBlocks = m / (t_DdrWidth * t_aRowMemWords);
  const unsigned int l_bColBlocks = p/ (t_DdrWidth * t_bColMemWords);
  const unsigned int l_aLd  = n / t_DdrWidth;
  const unsigned int l_bLd  = n/ t_DdrWidth;
  const unsigned int l_cLd  = n / t_DdrWidth;
  const unsigned int l_xLd 	= n / t_XDdrWidth;
  const int32_t l_postScale = 0;
  unsigned int l_transpBlocks = l_aColBlocks * l_aRowBlocks * l_bColBlocks *t_aRowMemWords;

	gemx_accel_wrapper( A, B, C, C, l_aColBlocks, l_aRowBlocks, l_bColBlocks, l_aLd, l_bLd, l_cLd, l_xLd, l_transpBlocks, l_postScale);

}

#define BLK_SIZE 128
#define min(a,b) (((a)<(b))?(a):(b))
void gemx(short* C, short * A, short* B, int m, int n, int p){
  int i, j, k, ii, jj, kk, Aik, bs = BLK_SIZE;

  for(ii = 0; ii < m; ii += bs)
    for(kk = 0; kk < n; kk += bs)
      for(jj = 0; jj < p; jj += bs)
        for(i = ii; i < min(m, ii+bs); i++)
          for(k = kk; k < min(n, kk+bs); k++)
          {
            Aik = A[n*i+k];
            for(j = jj; j < min(p, jj+bs); j+=8)
            {
              C[p*i+j] += Aik * B[p*k+j];
              C[p*i+j+1] += Aik * B[p*k+j+1];		
              C[p*i+j+2] += Aik * B[p*k+j+2];	
              C[p*i+j+3] += Aik * B[p*k+j+3];	
              C[p*i+j+4] += Aik * B[p*k+j+4];		
              C[p*i+j+5] += Aik * B[p*k+j+5];	
              C[p*i+j+6] += Aik * B[p*k+j+6];	
              C[p*i+j+7] += Aik * B[p*k+j+7];	
            }
          }					
}

#define MATRIX_SIZE 256
  short a[MATRIX_SIZE*MATRIX_SIZE] __attribute__ ((__aligned__(64)));
  short b[MATRIX_SIZE*MATRIX_SIZE] __attribute__ ((__aligned__(64)));
  short c[MATRIX_SIZE*MATRIX_SIZE] __attribute__ ((__aligned__(64)));

int main (){

  int64_t begin, end, dur;

  int i, j, k;
  printf("Start init.\n");
  for (i = 0; i < 100 ; i++){
    a[i] = i;
    b[i * MATRIX_SIZE] = i;
    b[i * MATRIX_SIZE + i] = i+40;
    //b[i + 40] = i;
  }
  printf("End init.\n");

  begin =  read_cycle();
#ifdef CUSTOM_DRIVER
  asm volatile ("fence.i");
  //gemx_accel(in_buffer, out_buffer);
  gemx_accel(c, a, b, MATRIX_SIZE, MATRIX_SIZE, MATRIX_SIZE);
  asm volatile ("fence.i");
#else

  // gemx(char* C, char* A, int* B, int m, int n, int p) 

  gemx(c, a, b, MATRIX_SIZE, MATRIX_SIZE, MATRIX_SIZE);
#endif 
  end =  read_cycle();	
  duration(begin, end);
  //printf("char out_buffer[MAX_FILE_SIZE] = {");
  
  for(i = 0; i < 10; i++){
    printf("c[%d] = %d\t", i, c[i]);
  }
  printf("\n");
  return 0;
}
