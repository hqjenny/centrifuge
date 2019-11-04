//#pragma once
#include "rocc.h"
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
#include "../os_utils.h"

// Total exp of actual size
#define EXP 9

// Total exp of arr buffer size
#define LIMIT 4
//#include "bitonic_accel.cpp"
#include "bitonic.h"

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
volatile int block;
begin = read_cycle();
#ifdef CUSTOM_INST
    #define XCUSTOM_ACC 1
    ROCC_INSTRUCTION_DSS(XCUSTOM_ACC, block, vtop_translate(h_a), vtop_translate(dir), 0);
    //ROCC_INSTRUCTION_DSS(XCUSTOM_ACC, block, h_a, dir, 0);
#else
    sort(h_a, dir);
#endif 
end = read_cycle();
duration(begin, end);


		int err = 0;
//		for (i = 0 ; i < SIZE; i++)
//		{
//      printf("array[%d]=%d\n", i, h_a[i]);
//			if(i+1 == SIZE)
//				break;
//			if(( h_a[i] > h_a[i + 1])!=dir){
//				err++;
//			}
//
//		}

    printf("There is/are %d error(s).\n", err);
		if(err!=0)
			return 1;
		return 0;
}
