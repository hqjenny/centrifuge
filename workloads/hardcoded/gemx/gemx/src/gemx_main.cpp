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
 *  @brief Main executable for SDX flow
 *
 *  $DateTime: 2018/02/16 14:56:29 $
 */

// Fast Csim compile
//   make host

// Fast run on board
//  ( gdb --args ./gemx.exe k app.bin app_out.bin )
 
// Fast CPU emu
// ( setenv XCL_EMULATION_MODE true ; ./out_host/gemx_host.exe out_cpu_emu/gemx.xclbin out_host/app.bin out_cpu_emu/app_out.bin )
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
//#include <stdlib.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <iomanip>

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#define ACCEL_CONTROL 0x20000
#define AP_DONE_MASK 0b10

#define ACCEL_INT 0x20004
#define ACCEL_A 0x20010
#define ACCEL_B 0x2001c
#define IN 0
#define OUT 1

#include "gemx_kernel.h"
#if TEST_SDX
  #include "gemx_fpga.h"
#endif

#include "rocc.h"
uint64_t read_cycle() {
	uint64_t rd = 0;
	asm volatile("rdcycle %0 " : "=r"(rd));
	return rd;
}	

uint64_t  vtop_translate(uint64_t src){
    #define XCUSTOM_ACC 3
    asm volatile ("fence.i");
    uint64_t ret;
    ROCC_INSTRUCTION_DS(XCUSTOM_ACC, ret, src, 0);
    asm volatile ("fence.i");      
    printf ("Translate Addr VA %llx to PA %llx\t", src, ret);
    return ret;
}

int access_addr(uint64_t gpio_addr, int direction, int value)
{
	int c;
	int fd;
	//int direction=IN;
	//unsigned gpio_addr = 0;
	//int value = 0;
	
	uint64_t page_addr, page_offset;
	void *ptr;
	uint64_t page_size=sysconf(_SC_PAGESIZE);

	printf("GPIO access through /dev/mem.\n", page_size);

	if (gpio_addr == 0) {
		printf("GPIO physical address is required.\n");
		return -1;
	}
	
	/* Open /dev/mem file */
	fd = open ("/dev/mem", O_RDWR);
	if (fd < 1) {
		return -1;
	}

	/* mmap the device into memory */
	page_addr = (gpio_addr & (~(page_size-1)));
	page_offset = gpio_addr - page_addr;
	ptr = mmap(NULL, page_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, page_addr);

	if (direction == IN) {
	/* Read value from the device register */
		value = *((unsigned *)(ptr + page_offset));
		printf("gpio dev-mem test: input: %08x\n",value);
	} else {
	/* Write value to the device register */
		*((unsigned *)(ptr + page_offset)) = value;
	}
	munmap(ptr, page_size);
	close(fd);

	return value;
}

int gemx_accel(char* a, char* b){

	uint64_t addr;
    // Disable interrupt for now
    //reg_write32(ACCEL_INT, 0x0);
	access_addr(ACCEL_INT, OUT, 0);

	 int status = access_addr(ACCEL_CONTROL, IN, 0);
	 printf("Status: %x\n", status);

    // Set up pointer a and pointer b address
    //reg_write32(ACCEL_A, (uint32_t)a);
	addr = vtop_translate(a);
	access_addr(ACCEL_A, OUT, addr);
	access_addr(ACCEL_A + 4, OUT, addr >> 32);
    //reg_write32(ACCEL_B, (uint32_t)b);
 	addr = vtop_translate(b);
	access_addr(ACCEL_B, OUT, addr);
	access_addr(ACCEL_B + 4, OUT, addr >> 32);

  asm volatile ("fence.i");

    // Write to ap_start to start the execution 
    //reg_write32(ACCEL_CONTROL, 0x1);
	access_addr(ACCEL_CONTROL, OUT, 0x1);

  asm volatile ("fence.i");
    //printf("Accel Control: %x\n", reg_read32(ACCEL_CONTROL));
	 status = access_addr(ACCEL_CONTROL, IN, 0);
	 printf("Status: %x\n", status);



    // Done?
    int done = 0;
    while (!done){
        //done = reg_read32(ACCEL_CONTROL) & AP_DONE_MASK;
	done = access_addr(ACCEL_CONTROL, IN, 0x1) & AP_DONE_MASK;
    }
    return 0;
}

std::ifstream::pos_type getFileSize(std::string p_FileName)
{
  std::ifstream in(p_FileName.c_str(), std::ifstream::ate | std::ifstream::binary);
  return in.tellg(); 
}

std::vector<DdrType>
loadBinFile(std::string p_BinFileName)
{
  std::vector<DdrType> l_memVec;
  // Bin file existence
  std::ifstream l_if(p_BinFileName.c_str(), std::ios::binary);
  if (l_if.is_open()) {
    // Bin file size
    size_t l_binFileSize = getFileSize(p_BinFileName);
    std::cout << "INFO: loading " + p_BinFileName + " of size " << l_binFileSize << "\n";
    assert(l_binFileSize > 0);
    size_t l_binFileSizeInDdrWords = l_binFileSize / sizeof(DdrType);
    assert(l_binFileSize % sizeof(DdrType) == 0);

    // Bin file storage
    //l_memVec.reserve(l_binFileSizeInDdrWords);
    l_memVec.resize(l_binFileSizeInDdrWords);
    DdrType *l_mem = &l_memVec[0];

    // Read the bin file
    l_if.read((char*)l_mem, l_binFileSize);


    // Pin the physical pages 

     if (mlock((char*)l_mem, l_binFileSize)) {
	  std::cout << "ERROR: mlock return null" << std::endl;

      	l_memVec.clear();
     }
   
    if (l_if) {
      std::cout << "INFO: loaded " << l_binFileSize << " bytes from " << p_BinFileName << "\n";
    } else {
      l_memVec.clear();
      std::cout << "ERROR: loaded only " << l_if.gcount() << " bytes from " << p_BinFileName << "\n";
    }
    l_if.close();

    // Debug print the file content
  } else {
    std::cout << "ERROR: failed to open file " + p_BinFileName + "\n";
  }

  return(l_memVec);
}

bool
writeBinFile(std::string p_BinFileName, std::vector<DdrType> &p_MemVec)
{
  bool ok = false;  
  std::ofstream l_of(p_BinFileName.c_str(), std::ios::binary);
  if (l_of.is_open()) {
    size_t l_sizeBytes =  sizeof(DdrType) * p_MemVec.size();
    l_of.write((char*)&p_MemVec[0], l_sizeBytes);
    if (l_of.tellp() == l_sizeBytes) {
      std::cout << "INFO: wrote " << l_sizeBytes << " bytes to " << p_BinFileName << "\n";
      ok = true;
    } else {
      std::cout << "ERROR: wrote only " << l_of.tellp() << " bytes to " << p_BinFileName << "\n";
    }
    l_of.close();
    munlock((char*)&p_MemVec[0], l_sizeBytes);
  }
  return(ok);
}

#if TEST_SDX
  typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePointType;

  void
  showTimeData(std::string p_Task, TimePointType &t1, TimePointType &t2)
  {
    t2 = std::chrono::high_resolution_clock::now();    
    std::chrono::duration<double> l_durationSec = t2 - t1;
    std::cout << "  DATA: time " << p_Task
              << "  " << std::fixed << std::setprecision(6)
              << l_durationSec.count() * 1e3 << " msec\n";
  }
#endif

int main(int argc, char** argv)
{
  if (argc < 4){
    printf("ERROR: passed %d arguments instead of %d, exiting\n",
           argc, 4);
    printf("  Usage:\n    gemx_host.exe  gemx.xclbin  app.bin  app_out.bin\n");
    return EXIT_FAILURE;
  }
  
  std::string l_xclbinFile(argv[1]);
  std::string l_binFile(argv[2]);
  std::string l_binFileOut(argv[3]);
  unsigned int l_kernelId = 0;
  unsigned int l_kernelNameId = 0;
  /*if (argc > 4) {
    l_kernelId = atoi(argv[4]);
    assert(l_kernelId < 4);
  }
  if (argc > 5) {
	l_kernelNameId = atoi(argv[5]);
	assert(l_kernelNameId < 4);
  }
  else {
	l_kernelNameId = l_kernelId;
  }*/

  printf("GEMX:   %s  %s  %s %s\n",
         argv[0], l_xclbinFile.c_str(), l_binFile.c_str(), l_binFileOut.c_str());
  
  // Load the bin file
  std::vector<DdrType> l_memVec = loadBinFile(l_binFile);
  if (l_memVec.empty()) {
    return EXIT_FAILURE;
  }
  DdrType *l_mem = &l_memVec[0];
  
  
  std::vector<DdrType> l_memVecOut[GEMX_numKernels];
  #if TEST_SDX
//    #include <chrono>
//    TimePointType l_tp[10];
//    unsigned int l_tpIdx = 0;
//    l_tp[l_tpIdx] = std::chrono::high_resolution_clock::now(); 
//    
//    // ################# HW run through SDX #################
//    // Init FPGA
//    gemx::Fpga l_fpga;
//    std::string kernelNames[GEMX_numKernels];
//    for (int i=0; i<GEMX_numKernels; ++i){
//	kernelNames[i] = "gemxKernel_" + std::to_string(i);
//    }
//    //std::string l_kernelName("gemxKernel_"+std::to_string(l_kernelNameId));
//    if (l_fpga.loadXclbinWithoutEvent(l_xclbinFile, kernelNames)) {
//      std::cout << "INFO: created kernels" << std::endl;
//    } else {
//      std::cerr << "ERROR: failed to load " + l_xclbinFile + "\n";
//      return EXIT_FAILURE;
//    }
//    showTimeData("loadXclbin", l_tp[l_tpIdx], l_tp[l_tpIdx+1]); l_tpIdx++;
//
//    // Transfer data to FPGA
//    gemx::MemDesc l_memDesc(l_memVec.size() * sizeof(DdrType) / GEMX_pageSizeBytes, l_memVec.data());
//    assert(l_memVec.size() * sizeof(DdrType) % GEMX_pageSizeBytes == 0);
//    if (l_fpga.copyToFpgaWithoutEvent(l_memDesc)) {
//      std::cout << "INFO: transferred data to FPGA" << std::endl;
//    } else {
//      std::cerr << "ERROR: failed to copy data to FPGA DDR\n";
//      return EXIT_FAILURE;
//    }
//    showTimeData("copyToFpga", l_tp[l_tpIdx], l_tp[l_tpIdx+1]); l_tpIdx++;
//
//    // Gemx kernel ops
//    if (l_fpga.callKernelWithoutEvent()) {
//      std::cout << "INFO: Executed kernel" << std::endl;
//    } else {
//      std::cerr << "ERROR: failed to call kernel \n";
//      return EXIT_FAILURE;
//    }
//    showTimeData("callKernel", l_tp[l_tpIdx], l_tp[l_tpIdx+1]); l_tpIdx++;
//  
//    gemx::MemDesc l_memDescOuts[GEMX_numKernels];
//    for (unsigned int i=0; i<GEMX_numKernels; ++i) {
//      l_memVecOut[i].resize(l_memVec.size());
//      gemx::MemDesc l_memDescOut(l_memDesc.sizePages(), l_memVecOut[i].data());
//      l_memDescOuts[i]= l_memDescOut;
//    }
//    // Transfer data back to host
//    if (l_fpga.copyFromFpgaWithoutEvent(l_memDescOuts)) {
//      std::cout << "INFO: Transferred data from FPGA" << std::endl;
//    } else {
//      std::cerr << "ERROR: failed to copy data from FPGA DDR\n";
//      return EXIT_FAILURE;
//    }    
//    showTimeData("copyFromFpga", l_tp[l_tpIdx], l_tp[l_tpIdx+1]); l_tpIdx++;
//    showTimeData("total", l_tp[0], l_tp[l_tpIdx]); l_tpIdx++;
//    showTimeData("subtotalFpga", l_tp[1], l_tp[l_tpIdx]); l_tpIdx++; // Host->DDR, kernel, DDR->host
//    
  #else
    // ################# SW run through HLS #################
    // Gemx kernel ops
   // gemxKernel_0(l_mem, l_mem);
    

    gemx_accel((char*)l_mem, (char*)l_mem);
    l_memVecOut[0] = l_memVec;
  
  #endif
  
  // Write out the received data
 for (int i=0; i<GEMX_numKernels; ++i) {
    std::size_t pos0 = l_binFileOut.find("/");
    std::size_t pos1 = l_binFileOut.find("app_out");
    std::size_t pos2 = l_binFileOut.find(".bin");
    std::size_t size_pos = pos2-pos1;
    //std::string binFileOutName =l_binFileOut.substr(0,pos0+1)+l_binFileOut.substr(pos1,size_pos) + std::to_string(i) + l_binFileOut.substr(pos2,4);
    std::string binFileOutName =l_binFileOut.substr(0,pos2)+ std::to_string(i) + l_binFileOut.substr(pos2,4);
   if (!writeBinFile(binFileOutName, l_memVecOut[i])) {
      std::cerr << "ERROR: failed to write output file " + binFileOutName + "\n";
      return EXIT_FAILURE;
    }
 }

  return EXIT_SUCCESS;
}

  
