#include <stdlib.h> 
#include <stdio.h> 
#include "mmio.h"
#include "time.h"
#include "gemx_params.h"
#include "bm_wrapper.h"
#define ACCEL_CONTROL 0x20000
#define ACCEL_BASE 0x20000
#define AP_DONE_MASK 0b10

#define ACCEL_INT 0x20004

void gemx_accel(short* C, short * A, short* B, int m, int n, int p)
{
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
void gemx(short* C, short * A, short* B, int m, int n, int p)
{
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

sls_mem_t *sls_state;
int main (){

  int64_t begin, end, dur;

  int i, j, k;

#ifdef CUSTOM_DRIVER
  sls_init(GPIO_BASE);
#endif

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
