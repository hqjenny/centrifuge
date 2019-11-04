#ifndef GEMX_PARAMS_H
#define GEMX_PARAMS_H

// Default settings 
#define GEMX_dataType       short
#define GEMX_dataEqIntType  short
#define GEMX_ddrWidth        32  
#define GEMX_XdataType      int32_t 
#define GEMX_XddrWidth      16  
#define GEMX_argInstrWidth    1   
#define GEMX_numInstr        16  

#define GEMX_gemvkVectorBlocks   512 
#define GEMX_gemvmVectorBlocks   512 
#define GEMX_gemvmGroups          1
 

#define GEMX_gemmMBlocks         1 
#define GEMX_gemmKBlocks         2
#define GEMX_gemmNBlocks         1
#define GEMX_splitMesh           0

#define GEMX_keepMacBits         0
#define GEMX_macBits             48

#define GEMX_transpBlocks        1

#define GEMX_spmvWidth           8 
#define GEMX_spmvkVectorBlocks   2048
#define GEMX_spmvMacGroups       12  
#define GEMX_spmvPadA            1
#define GEMX_spmvNumCblocks      1024
#define GEMX_spmvFloatPerDesc    4

// Correlated for IdxBits 2 > row idx < 2**14 so blocks 10 (2**14 / ddrw / spmvw / groups
#define GEMX_spmvColAddIdxBits   2


#define GEMX_argPipeline         2
#define GEMX_part                vcu1525
#define GEMX_kernelHlsFreq       250
#define GEMX_kernelVivadoFreq    250
// How many kernels to replicate in the accelerator (use 1 to 4)
#define GEMX_numKernels          1
// What engines get included in each accelerator kernel (use 0 or 1)
// The more engines you include the more catability you get but P&R
// P&R becomes more difficult thus you get lower Fmax
#define GEMX_runGemv             0
#define GEMX_runGemm             1
#define GEMX_runTransp           1
#define GEMX_runSpmv             0

// Defauts for SPMV 32-wide
//#if GEMX_ddrWidth ==  32
//  #define  GEMX_spmvWidth 8
//#endif
//
// #if GEMX_dataType == float
//  #define  GEMX_dataEqIntType     int
//
//  #define GEMX_gemvmVectorBlocks  43
//
//  #define GEMX_spmvPadA          0
//  #define GEMX_spmvFloatPerDesc  2
//  #define GEMX_spmvMacGroups       12
//    #if GEMX_ddrWidth == 16
//       #define GEMX_spmvWidth        8
//   // #GEMX_spmvmVectorBlocks    10
//  #endif
//#endif

#endif
