#ifndef __OS_UTILS_H__
#define __OS_UTILS_H__
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "rocc.h"
#define IN 0
#define OUT 1

typedef struct sls_mem {
  /* GPIO */
  int fd; // file descriptor mapping /dev/mem
  void *gpio; // mmaped physical mem
  uintptr_t gpio_base; // base address for MMIO

  /* MAPPED KERNEL MEM */
  uint8_t *a_buf; // mmaped accelerator buffer
  size_t a_buf_sz; // size of mmaped region
} sls_mem_t;

sls_mem_t *sls_init(uintptr_t gpio_base)
{
  sls_mem_t *mem = malloc(sizeof(sls_mem_t));
  mem->gpio_base = gpio_base;

  /* /dev/mem lets us access physical memory */
  mem->fd = open("/dev/mem", O_RDWR | O_SYNC);
  if(mem->fd < 1) {
    fprintf(stderr, "FAILURE: failed to initialize accelerator interface\n");
    return NULL;
  }
  
  /* We assume that device MMIO is page aligned and fits in one page */
	unsigned page_size = sysconf(_SC_PAGESIZE);
	uintptr_t page_addr = (mem->gpio_base & (~(page_size-1)));
	mem->gpio = mmap(NULL, page_size, PROT_READ|PROT_WRITE, MAP_SHARED, mem->fd, gpio_base);
  if(mem->gpio == NULL) {
    fprintf(stderr, "FAILURE: failed to initialize accelerator interface\n");
    return NULL;
  }
}

uint64_t read_cycle() {
	uint64_t rd = 0;
	asm volatile("rdcycle %0 " : "=r"(rd));
	return rd;
}	

uint64_t duration(uint64_t start, uint64_t end) {
	uint64_t dur = end - start;	
	printf("Duration: %ld. \n", dur);
	return dur;
}

uint64_t  vtop_translate(uint64_t src) {
  #define XCUSTOM_ACC 3
  //asm volatile ("fence.i");
  uint64_t ret;
  ROCC_INSTRUCTION_DS(XCUSTOM_ACC, ret, src, 0);
  //asm volatile ("fence.i");      
  //printf ("Translate Addr VA %llx to PA %llx\t", src, ret);
  return ret;
}

static int fd;
int open_dev_mem(){
	/* Open /dev/mem file */
	fd = open ("/dev/mem", O_RDWR);
	if (fd < 1) {
		return -1;
	}
}

int close_dev_mem(){
	close(fd);
}

int access_addr(unsigned gpio_addr, int direction, int value)
{

	if(fd < 1) {
    fd = open ("/dev/mem", O_RDWR | O_SYNC); 
	}
	int c;
	//int fd;
	//int direction=IN;
	//unsigned gpio_addr = 0;
	//int value = 0;
	
	unsigned page_addr, page_offset;
	void *ptr;
	unsigned page_size=sysconf(_SC_PAGESIZE);

	//printf("GPIO access through /dev/mem.\n", page_size);

	if (gpio_addr == 0) {
		//printf("GPIO physical address is required.\n");
		return -1;
	}
	

	/* mmap the device into memory */
	page_addr = (gpio_addr & (~(page_size-1)));
	page_offset = gpio_addr - page_addr;
	ptr = mmap(NULL, page_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, page_addr);

	if (direction == IN) {
	/* Read value from the device register */
		value = *((unsigned *)(ptr + page_offset));
		//printf("gpio dev-mem test: input: %08x\n",value);
	} else {
	/* Write value to the device register */
		*((unsigned *)(ptr + page_offset)) = value;
	}
	munmap(ptr, page_size);

	return value;
}
#endif
