#ifndef __OS_UTILS_H__
#define __OS_UTILS_H__
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#define IN 0
#define OUT 1

static inline uint64_t read_cycle() {
	uint64_t rd = 0;
	asm volatile("rdcycle %0 " : "=r"(rd));
	return rd;
}	

static inline uint64_t duration(uint64_t start, uint64_t end) {
	uint64_t dur = end - start;	
	printf("Duration: %ld. \n", dur);
	return dur;
}

// Generic translation of userspace vaddr to paddr (hard-coded to 4k pages)
uintptr_t vtop(uintptr_t vaddr);

// Access an MMIO address
// XXX Needs to be updated to work with the rest of libcf
// int access_addr(unsigned gpio_addr, int direction, int value);

#endif
