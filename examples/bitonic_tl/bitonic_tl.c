//#pragma once
/*----------------------------------------------------------------------------
*
* Author:   Liang Ma (liang-ma@polito.it)
*
*----------------------------------------------------------------------------
*/
#ifdef CUSTOM_DRIVER
#include "bm_wrapper.h"
#include "rocc.h"
#endif
#include "time.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
//#define GLOBAL

// Total exp of actual size
#define EXP 3

// Total exp of arr buffer size
#define LIMIT 2
//#include "bitonic_accel.cpp"
#include "bitonic.h"

#ifdef CUSTOM_DRIVER
uint64_t  vtop_translate(uint64_t src){
    #define XCUSTOM_ACC 3
    //asm volatile ("fence.i");
    uint64_t ret;
    ROCC_INSTRUCTION_DS(XCUSTOM_ACC, ret, src, 0);
    //asm volatile ("fence.i");      
    printf ("Translate Addr VA %ld to PA %ld\t", src, ret);
    return ret;
}
#endif 

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
    sort_wrapper(h_a, dir);
#else
    sort(h_a, dir);
#endif 
end = read_cycle();
duration(begin, end);

		int err = 0;
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
