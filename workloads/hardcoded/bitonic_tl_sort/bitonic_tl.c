//#pragma once

#include "../os_utils.h"
#define ACCEL_CONTROL 0x30000
#define AP_DONE_MASK 0b10

#define ACCEL_INT 0x30004
#define ACCEL_SRC 0x30010 
#define ACCEL_DIR 0x3001c
/*----------------------------------------------------------------------------
*
* Author:   Liang Ma (liang-ma@polito.it)
*
*----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
//#define GLOBAL

// Total exp of actual size
#define EXP 3

// Total exp of arr buffer size
#define LIMIT 2
//#include "bitonic_accel.cpp"
#include "bitonic.h"

#define ARRAY_SIZE 8
int sort_accel (int* src, int dir){

    //uint64_t src_pa = vtop_translate((uint64_t)src);
    uint64_t src_pa = (uint64_t)src;

    // Disable interrupt for now
    //reg_write32(ACCEL_INT, 0x0);

	  access_addr(ACCEL_INT, OUT, 0);

	uint64_t addr;
	addr = vtop_translate(src);
	access_addr(ACCEL_SRC, OUT, addr);
	access_addr(ACCEL_SRC + 4, OUT, addr >> 32);
	
	access_addr(ACCEL_DIR, OUT, dir);

    // Set up pointer a and pointer b address
   // reg_write32(ACCEL_SRC, (uint64_t)src_pa);
   // reg_write32(ACCEL_DIR, (uint32_t)dir);

    // Write to ap_start to start the execution 
	access_addr(ACCEL_CONTROL, OUT, 0x1);
    //reg_write32(ACCEL_CONTROL, 0x1);
    //printf("Accel Control: %x\n", reg_read32(ACCEL_CONTROL));

    // Done?
    int done = 0;
    while (!done){
       // done = reg_read32(ACCEL_CONTROL) & AP_DONE_MASK;
	    done = access_addr(ACCEL_CONTROL, IN, 0x1) & AP_DONE_MASK;
    }   
    return 0;
}

int main(int argc, char** argv)
{
		// input and output parameters
		int SIZE = ARRAY_SIZE;
		typedef int TYPE;
    TYPE h_a[SIZE];

		int dir = 1;

    int i = 0;
		for(i = 0;i<SIZE;i++)
		{
			//h_a[i]=rand()%(100*ARRAY_SIZE);
      h_a[i] = i;
		}

uint64_t begin, end, dur;
begin = read_cycle();
#ifdef CUSTOM_DRIVER
    sort_accel(h_a, dir);
#else
    sort(h_a, dir);
#endif 
end = read_cycle();
duration(begin, end);
int err;
		for (i = 0 ; i < SIZE; i++)
		{
      printf("array[%d]=%d\n", i, h_a[i]);
			if(i+1 == SIZE)
				break;
			if(( h_a[i] > h_a[i + 1])!=dir){
				err++;
			}

		}

    printf("There is/are %d error(s).\n", err);
		if(err!=0)
			return 1;
		return 0;
}
