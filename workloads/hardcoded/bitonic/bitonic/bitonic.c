//#pragma once
/*----------------------------------------------------------------------------
*
* Author:   Liang Ma (liang-ma@polito.it)
*
*----------------------------------------------------------------------------
*/

#include "time.h"
#ifdef CUSTOM_INST
#include "bm_wrapper.h"
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
//#define GLOBAL

// Total exp of actual size
#define EXP 3

// Total exp of arr buffer size
#define LIMIT 2
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

    uint64_t begin, end;
    begin = read_cycle();
#ifdef CUSTOM_INST
    sort_wrapper((uint64_t)h_a, (uint64_t)dir);
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
