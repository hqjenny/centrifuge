#ifndef __TIME_H__
#define __TIME_H__

#include<stdio.h>
#include<stdint.h>
//#include "encoding.h"

uint64_t read_cycle() {
	uint64_t rd = 0;
	asm volatile("rdcycle %0 " : "=r"(rd));	
	//rd = rdcycle();
	//printf("Time: %ld. \n", rd);
	return rd;
}	

uint64_t duration(uint64_t start, uint64_t end){	
	uint64_t dur = end - start;	
	printf("\nDURATION: %ld\n", dur);
	return dur;
}

#endif
