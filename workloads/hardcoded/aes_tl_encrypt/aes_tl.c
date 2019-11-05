/*
+--------------------------------------------------------------------------+
| CHStone : a suite of benchmark programs for C-based High-Level Synthesis |
| ======================================================================== |
|                                                                          |
| * Collected and Modified : Y. Hara, H. Tomiyama, S. Honda,               |
|                            H. Takada and K. Ishii                        |
|                            Nagoya University, Japan                      |
|                                                                          |
| * Remark :                                                               |
|    1. This source code is modified to unify the formats of the benchmark |
|       programs in CHStone.                                               |
|    2. Test vectors are added for CHStone.                                |
|    3. If "main_result" is 0 at the end of the program, the program is    |
|       correctly executed.                                                |
|    4. Please follow the copyright of each benchmark program.             |
+--------------------------------------------------------------------------+
*/
/* aes.c */
/*
 * Copyright (C) 2005
 * Akira Iwata & Masayuki Sato
 * Akira Iwata Laboratory,
 * Nagoya Institute of Technology in Japan.
 *
 * All rights reserved.
 *
 * This software is written by Masayuki Sato.
 * And if you want to contact us, send an email to Kimitake Wakayama
 * (wakayama@elcom.nitech.ac.jp)
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software must
 *    display the following acknowledgment:
 *    "This product includes software developed by Akira Iwata Laboratory,
 *    Nagoya Institute of Technology in Japan (http://mars.elcom.nitech.ac.jp/)."
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Akira Iwata Laboratory,
 *     Nagoya Institute of Technology in Japan (http://mars.elcom.nitech.ac.jp/)."
 *
 *   THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 *   AKIRA IWATA LABORATORY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 *   SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 *   IN NO EVENT SHALL AKIRA IWATA LABORATORY BE LIABLE FOR ANY SPECIAL,
 *   INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 *   FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 *   NEGLIGENCE OR OTHER TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION
 *   WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
#include <stdio.h>

//#include "mmio.h"
#include "time.h"
#include "../os_utils.h"
#define ACCEL_CONTROL 0x30000
#define AP_DONE_MASK 0b10

#define ACCEL_INT 0x30004
#define ACCEL_STATEMT 0x30018
#define ACCEL_KEY 0x30024
#define ACCEL_TYPE 0x30030
#define ACCEL_RET 0x30010 

int main_result;

#include "aes.h"
#include "aes_enc.c"
#include "aes_dec.c"
#include "aes_key.c"
#include "aes_func.c"
#include "../custom_mmap/mmap_driver.c"

int encrypt_accel(int* statemt, int* key, int type){

	uint64_t addr;
    // Disable interrupt for now
    //reg_write32(ACCEL_INT, 0x0);
	  access_addr(ACCEL_INT, OUT, 0);

        int fd1 = mmap_init();
        int fd2 = mmap_init();
        char * addr1 = copy_to_buffer((char*)statemt, 32 * sizeof(int), fd1);

	addr = vtop_translate(statemt);
   addr =  vtop_translate(addr1);
	access_addr(ACCEL_STATEMT, OUT, addr);
	access_addr(ACCEL_STATEMT + 4, OUT, addr >> 32);
	
        char * addr2 = copy_to_buffer((char*)key, 32 * sizeof(int), fd2);
	//addr = vtop_translate(key);
	addr = vtop_translate(addr2);
	access_addr(ACCEL_KEY, OUT, addr);
	access_addr(ACCEL_KEY + 4, OUT, addr >> 32);

	//addr = vtop_translate(key);
	access_addr(ACCEL_TYPE, OUT, type);

    // Set up pointer a and pointer b address
//    reg_write32(ACCEL_STATEMT, (uint32_t)statemt);
//    reg_write32(ACCEL_KEY, (uint32_t)key);
//    reg_write32(ACCEL_TYPE, (uint32_t)type);

    // Write to ap_start to start the execution 
	access_addr(ACCEL_CONTROL, OUT, 0x1);
  //  reg_write32(ACCEL_CONTROL, 0x1);
    //printf("Accel Control: %x\n", reg_read32(ACCEL_CONTROL));

    // Done?
    int done = 0;
    while (!done){
      //  done = reg_read32(ACCEL_CONTROL) & AP_DONE_MASK;

	    done = access_addr(ACCEL_CONTROL, IN, 0x1) & AP_DONE_MASK;
    }  
    memcpy(statemt, addr1, 32 * 4 );
    memcpy(key, addr2, 32 * 4 );
    
    mmap_delete(fd1, addr1);
    mmap_delete(fd2, addr2);
 
    //int c = reg_read32(ACCEL_RET);
    return 0;
}


/* ***************** main **************************** */
int
aes_main (void)
{
/*
+--------------------------------------------------------------------------+
| * Test Vectors (added for CHStone)                                       |
|     statemt, key : input data                                            |
+--------------------------------------------------------------------------+
*/
  statemt[0] = 50;
  statemt[1] = 67;
  statemt[2] = 246;
  statemt[3] = 168;
  statemt[4] = 136;
  statemt[5] = 90;
  statemt[6] = 48;
  statemt[7] = 141;
  statemt[8] = 49;
  statemt[9] = 49;
  statemt[10] = 152;
  statemt[11] = 162;
  statemt[12] = 224;
  statemt[13] = 55;
  statemt[14] = 7;
  statemt[15] = 52;

  key[0] = 43;
  key[1] = 126;
  key[2] = 21;
  key[3] = 22;
  key[4] = 40;
  key[5] = 174;
  key[6] = 210;
  key[7] = 166;
  key[8] = 171;
  key[9] = 247;
  key[10] = 21;
  key[11] = 136;
  key[12] = 9;
  key[13] = 207;
  key[14] = 79;
  key[15] = 60;

int64_t begin, end, dur;
begin =  read_cycle();
#ifdef CUSTOM_DRIVER
  encrypt_accel(statemt, key, 128128);
#else
  encrypt (statemt, key, 128128);
#endif 
end =  read_cycle();	
  decrypt (statemt, key, 128128);

duration(begin, end);
  return 0;
}


int
main ()
{
	open_dev_mem();
      main_result = 0;
      aes_main ();
      printf ("\n%d\n", main_result);
      close_dev_mem();
      return main_result;

    }
