/**********
 * Copyright (c) 2017, Xilinx, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * **********/
/**
 *  @brief Semi-standalone generator for the test task binary images (2nd arg to gemc.exe)
 *
 *  $DateTime: 2018/03/01 16:50:01 $
 */

// Fast compile and run:
//   make host

 
#include <stdio.h>
//#include <stdlib.h>
#include <string>
#include <vector>

#include <mm_malloc.h>
#include "gemx_gen_bin.h"



int main(int argc, char** argv)
{
  if (argc < 3 ){
    printf("ERROR: passed %d arguments instead of %d, exiting\n",
           argc, 3);
    std::cout << "  Usage:\n    gemx_gen_bin.exe  <-write | -read> app.bin [op1 arg arg ...] [op2 arg arg ...] ... | -compare tol_rel tol_abs app_gold.bin app_out.bin\n"
              << "    Ops:\n"
              << "      gemv   M K   LdA            HandleA HandleB HandleC\n"
              << "      gemm   M K N LdA  LdB  LdC LdX postScalVal postScaleShift HandleA HandleB HandleC HandleX\n"
              << "      transp M N   LdIn LdOut  FormatA FormatB  HandleA HandleB\n"
              << "      spmv   M K   Nnz  mtxFile   HandleA HandleB HandleC\n"
              << "    Examples:\n"
              << "      gemx_gen_bin.exe -write app.bin transp 32 32  32 32  rm cm  A0 B0\n"
              << "      gemx_gen_bin.exe -write app.bin transp  4 4 8 12  rm cm A0 B0  transp  64 96 128 144 rm cm A1 B1\n"
              << "      gemx_gen_bin.exe -write app.bin transp  4 4 8 12 rm gvfa A0 A1  gemv 4 4 12 A0 B0 C1\n"
              << "      gemx_gen_bin.exe -write app.bin spmv 8 8 16 none A0 B0 C0\n"
              << "      gemx_gen_bin.exe -read app_gold.bin\n"
              << "      gemx_gen_bin.exe -read app_gold.bin\n"
              << "      gemx_gen_bin.exe -compare 1e-3 1e-9 app_gold.bin app_out.bin\n"
              << "\n";
    return EXIT_FAILURE;
  }
  
  std::string l_mode(argv[1]);
  bool l_write = l_mode == "-write";
  bool l_read = l_mode == "-read";
  bool l_compare = l_mode == "-compare";
  float l_TolRel = 0, l_TolAbs = 0;
  
  std::string l_binFile[2];
  
  if (l_read || l_write) {
    l_binFile[0] = argv[2];
    l_binFile[1] = l_binFile[0].substr(0, l_binFile[0].find_last_of(".")) + "_gold.bin";

    printf("GEMX:  %s %s %s\n",
           argv[0], l_mode.c_str(), l_binFile[0].c_str());
  } else if (l_compare) {
    std::stringstream l_TolRelS(argv[2]);
    std::stringstream l_TolAbsS(argv[3]);
    l_TolRelS >> l_TolRel;
    l_TolAbsS >> l_TolAbs;
    l_binFile[0] = argv[4];
    l_binFile[1] = argv[5];
    printf("GEMX:  %s %s %g %g %s %s\n",
           argv[0], l_mode.c_str(),
           l_TolRel, l_TolAbs,
           l_binFile[0].c_str(), l_binFile[1].c_str());
  } else {
    assert(0);
  }
  
  // Early assert for proper instruction length setting
  assert(sizeof(GEMX_dataType) * GEMX_ddrWidth* GEMX_argInstrWidth == GEMX_instructionSizeBytes); 

  ////////////////////////  TEST PROGRAM STARTS HERE  ////////////////////////
  
  GenControl l_control;
  GenGemv l_gemv;
  GenGemm l_gemm;
  GenTransp l_transp;
  GenSpmv l_spmv;
  
  if (l_write) {
    ProgramType l_p[2];  // 0 - no golden, 1 with golden

    for(unsigned int wGolden = 0; wGolden < 2; ++wGolden) {
      
      unsigned int l_argIdx = 3;
      unsigned int l_instrCount = 0;
      
      while (l_argIdx < argc) {
        std::string l_opName(argv[l_argIdx++]);
        TimePointType l_t1 = std::chrono::high_resolution_clock::now(), l_t2;
        if (l_opName == "control") {
          bool l_isLastOp = atoi(argv[l_argIdx++]);
          bool l_noop = atoi(argv[l_argIdx++]);
          l_control.addInstr(l_p[wGolden], l_isLastOp, l_noop);
        } else if (l_opName == "gemv") {
          unsigned int l_m = atoi(argv[l_argIdx++]);
          unsigned int l_k = atoi(argv[l_argIdx++]);
          unsigned int l_lda = atoi(argv[l_argIdx++]);
          std::string l_handleA(argv[l_argIdx++]);
          std::string l_handleB(argv[l_argIdx++]);
          std::string l_handleC(argv[l_argIdx++]);
          if (!l_gemv.check(l_m, l_k, l_lda)) exit(1);
          l_gemv.addInstr(l_p[wGolden], l_m,  l_k, l_lda, l_handleA, l_handleB, l_handleC, wGolden);
        } else if (l_opName == "gemm") {
          unsigned int l_m = atoi(argv[l_argIdx++]);
          unsigned int l_k = atoi(argv[l_argIdx++]);
          unsigned int l_n = atoi(argv[l_argIdx++]);
          unsigned int l_lda = atoi(argv[l_argIdx++]);
          unsigned int l_ldb = atoi(argv[l_argIdx++]);
          unsigned int l_ldc = atoi(argv[l_argIdx++]);
	  unsigned int l_ldx = atoi(argv[l_argIdx++]);
	  int32_t     l_postScaleVal = atoi(argv[l_argIdx++]);
          int32_t     l_postScaleShift = atoi(argv[l_argIdx++]);
          int32_t     l_postScale = (l_postScaleVal << 8) | (l_postScaleShift & 0x000000ff);
          std::string l_handleA(argv[l_argIdx++]);
          std::string l_handleB(argv[l_argIdx++]);
          std::string l_handleC(argv[l_argIdx++]);
	  std::string l_handleX(argv[l_argIdx++]);
          assert(l_lda >= l_k);
          assert(l_ldb >= l_n);
          assert(l_ldc >= l_n);
	  assert(l_ldx >= l_n);
          if (!l_gemm.check(l_m, l_k, l_n, l_lda, l_ldb, l_ldc, l_ldx)) exit(1);
          l_gemm.addInstr(l_p[wGolden], l_m,  l_k, l_n, l_lda, l_ldb, l_ldc, l_ldx, l_postScale,
                          l_handleA, l_handleB, l_handleC, l_handleX, wGolden);
        } else if (l_opName == "transp") {
          unsigned int l_m = atoi(argv[l_argIdx++]);
          unsigned int l_n = atoi(argv[l_argIdx++]);
          unsigned int l_lda = atoi(argv[l_argIdx++]);
          unsigned int l_ldb = atoi(argv[l_argIdx++]);
          MatFormatType l_formatA = gemx_ns::DdrMatrixShape::string2format(argv[l_argIdx++]);
          MatFormatType l_formatB = gemx_ns::DdrMatrixShape::string2format(argv[l_argIdx++]);
          std::string l_handleA(argv[l_argIdx++]);
          std::string l_handleB(argv[l_argIdx++]);
          if (!l_transp.check(l_m, l_n, l_lda, l_ldb, l_formatA, l_formatB)) exit(1);
          if ((l_formatB == MatFormatType::GvA) && (l_ldb == 0)) {
            l_ldb = GEMX_ddrWidth * l_n;
          }
          assert(l_lda >= l_n);
          assert((l_ldb >= l_m) || (l_formatB == MatFormatType::GvA));
          l_transp.addInstr(l_p[wGolden], l_m, l_n, l_lda, l_ldb, l_formatA, l_formatB,
                            l_handleA, l_handleB, wGolden);
        } else if (l_opName == "spmv") {
          unsigned int l_m = atoi(argv[l_argIdx++]);
          unsigned int l_k = atoi(argv[l_argIdx++]);
          unsigned int l_nnz = atoi(argv[l_argIdx++]);
          std::string l_mtxFileName(argv[l_argIdx++]);
          std::string l_handleA(argv[l_argIdx++]);
          std::string l_handleB(argv[l_argIdx++]);
          std::string l_handleC(argv[l_argIdx++]);
          MtxFile l_mtxFile(l_mtxFileName);
          if (!l_spmv.check(l_m, l_k, l_nnz, l_mtxFile)) exit(1);
          l_spmv.addInstr(l_p[wGolden], l_m,  l_k, l_nnz, l_mtxFile,
                          l_handleA, l_handleB, l_handleC, wGolden);
       } else {
         std::cerr << "ERROR: unknow op \"" << l_opName << "\"\n";
       }
       l_instrCount++;
       assert(l_instrCount < GEMX_numInstr - 1); // 1 is for the mandatory control instruction
       assert(l_argIdx <= argc);
       if (wGolden) {
         showTimeData("  " + l_opName + " with golden took ", l_t1, l_t2);
       } else {
         std::cout << "\n";
       }
     }
     // Fill noops (workaround for HLS issue with dataflow loops)
     while (l_instrCount < GEMX_numInstr - 1) {
       l_control.addInstr(l_p[wGolden], false, true);
       std::cout << "\n";
       l_instrCount++;
     }
     
     l_control.addInstr(l_p[wGolden], true, false);
     std::cout << "\n";
     l_instrCount++;
     assert(l_instrCount == GEMX_numInstr);
     
     l_p[wGolden].writeToBinFile(l_binFile[wGolden]);
   }

  } else if (l_read) {
    
    // Read file
    ProgramType l_p;
    l_p.readFromBinFile(l_binFile[0]);

    // Show cycle counts
    KargsType l_kargsRes;
    std::cout << "\nINFO:   format "
              << std::right << std::setw(4)  << "op"
              << std::right << std::setw(12) << "start"
              << std::right << std::setw(12) << "end"
              << std::right << std::setw(12) << "duration"
              << std::right << std::setw(14) << "ms@250MHz"
              << "\n";
    for (unsigned int l_pc = 0; l_pc < GEMX_numInstr; ++l_pc) {
      KargsOpType l_op = l_kargsRes.load(l_p.getBaseResAddr(), l_pc * l_kargsRes.getInstrWidth());
      assert(l_op == KargsType::OpResult || l_op == KargsType::OpControl); // OpControl is 0 which is ok
      gemx_ns::InstrResArgs l_instrRes = l_kargsRes.getInstrResArgs();
      std::cout << "  DATA: cycles "
                << std::setw(4) << l_pc
                << std::setw(12) << l_instrRes.m_StartTime
                << std::setw(12) << l_instrRes.m_EndTime
                << std::setw(12) << l_instrRes.getDuration()
                << std::setw(14) << std::fixed << std::setprecision(6) << (l_instrRes.getDuration() / 250e6 * 1e3)
                << "\n";
    }
    std::cout << "\n";
    
    // Show all instructions
    KargsType l_kargs;
    unsigned int l_pc = 0;
    bool l_isLastOp = false;
    do {
      KargsOpType l_op = l_kargs.load(l_p.getBaseInstrAddr(), l_pc);
      switch(l_op) {
        case KargsType::OpControl: {
          ControlArgsType l_controlArgs = l_kargs.getControlArgs();
          l_isLastOp = l_controlArgs.getIsLastOp();
          bool l_noop = l_controlArgs.getNoop();
          assert(l_isLastOp || l_noop);
          break;
        }
        case KargsType::OpGemv: {
          GemvArgsType l_gemvArgs = l_kargs.getGemvArgs();
          l_gemv.show(l_p, l_gemvArgs);
          break;
        }
        case KargsType::OpGemm: {
          GemmArgsType l_gemmArgs = l_kargs.getGemmArgs();
          l_gemm.show(l_p, l_gemmArgs);
          break;
        }
        case KargsType::OpTransp: {
          TranspArgsType l_transpArgs = l_kargs.getTranspArgs();
          l_transp.show(l_p, l_transpArgs);
          break;
        }
        case KargsType::OpSpmv: {
          SpmvArgsType l_spmvArgs = l_kargs.getSpmvArgs();
          l_spmv.show(l_p, l_spmvArgs);
          break;
        }
        default: {
          assert(false);
        }
      }
      l_pc += l_kargs.getInstrWidth();
    } while(!l_isLastOp);
    
  } else if (l_compare) {
    // Read files
    ProgramType l_p[2];
    l_p[0].readFromBinFile(l_binFile[0]);
    l_p[1].readFromBinFile(l_binFile[1]);
    
    // Compare all instructions
    KargsType l_kargs0, l_kargs1;
    unsigned int l_pc = 0;
    bool l_isLastOp = false;
    bool l_compareOk = true;
    do {
      KargsOpType l_op0 = l_kargs0.load(l_p[0].getBaseInstrAddr(), l_pc);
      KargsOpType l_op1 = l_kargs1.load(l_p[1].getBaseInstrAddr(), l_pc);
      if (l_op1 == KargsType::OpResult) {
        break;
      }
      assert(l_op0 == l_op1);
      switch(l_op0) {
        case KargsType::OpControl: {
          ControlArgsType l_controlArgs = l_kargs0.getControlArgs();
          l_isLastOp = l_controlArgs.getIsLastOp();
          break;
        }
        case KargsType::OpGemv: {
          GemvArgsType l_gemvArgs = l_kargs0.getGemvArgs();
          bool l_opOk = l_gemv.compare(l_TolRel, l_TolAbs, l_p[0], l_p[1], l_gemvArgs);
          l_compareOk = l_compareOk && l_opOk;
          break;
        }
        case KargsType::OpGemm: {
          GemmArgsType l_gemmArgs = l_kargs0.getGemmArgs();
          bool l_opOk = l_gemm.compare(l_TolRel, l_TolAbs, l_p[0], l_p[1], l_gemmArgs);
          l_compareOk = l_compareOk && l_opOk;
          break;
        }
        case KargsType::OpTransp: {
          TranspArgsType l_transpArgs = l_kargs0.getTranspArgs();
          bool l_opOk = l_transp.compare(l_TolRel, l_TolAbs, l_p[0], l_p[1], l_transpArgs);
          l_compareOk = l_compareOk && l_opOk;
          break;
        }
        case KargsType::OpSpmv: {
          SpmvArgsType l_spmvArgs = l_kargs0.getSpmvArgs();
          bool l_opOk = l_spmv.compare(l_TolRel, l_TolAbs, l_p[0], l_p[1], l_spmvArgs);
          l_compareOk = l_compareOk && l_opOk;
          break;
        }
        default: {
          assert(false);
        }
      }
      l_pc += l_kargs0.getInstrWidth();
    } while(!l_isLastOp);
    
    // Exit status from compare
    if (!l_compareOk) {
      return EXIT_FAILURE;
    }
  
  } else {
    assert(0); // Unknown user command
  }
  
  return EXIT_SUCCESS;
}

  
