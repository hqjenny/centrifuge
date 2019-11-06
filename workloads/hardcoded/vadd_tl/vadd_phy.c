#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#define LENGTH 80
#define IN 0
#define OUT 1

#include "rocc.h"
#define ACCEL_CONTROL 0x20000
#define AP_DONE_MASK 0b10

#define ACCEL_INT 0x20004
#define ACCEL_A 0x20018
#define ACCEL_B 0x20020
#define ACCEL_C 0x20028
#define ACCEL_LEN 0x20030
#define ACCEL_RET 0x20010 

uint64_t  vtop_translate(uint64_t src){
    #define XCUSTOM_ACC 3
    //asm volatile ("fence.i");
    uint64_t ret;
    ROCC_INSTRUCTION_DS(XCUSTOM_ACC, ret, src, 0);
    //asm volatile ("fence.i");      
    printf ("Translate Addr VA %llx to PA %llx\t", src, ret);
    return ret;
}

void print_vec(int* vec, int length){
    for(int i = 0; i < length; i++){
        if (i != 0 ) printf(", ");
        printf("%d", vec[i]);
    }
}

int access_addr(unsigned gpio_addr, int direction, int value, int print)
{
	int c;
	int fd;
	//int direction=IN;
	//unsigned gpio_addr = 0;
	//int value = 0;
	
	unsigned page_addr, page_offset;
	void *ptr;
	unsigned page_size=sysconf(_SC_PAGESIZE);
	if (print)
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
		if (print)
		printf("gpio dev-mem test: input: %08x\n",value);
	} else {
	/* Write value to the device register */
		*((unsigned *)(ptr + page_offset)) = value;
	}
	munmap(ptr, page_size);
	close(fd);

	return value;
}


int vadd_accel(int* a, int* b, int* c, int length){
	uint64_t addr;
    // Disable interrupt for now
    //reg_write32(ACCEL_INT, 0x0);
	access_addr(ACCEL_INT, OUT, 0, 1);
    // Set up pointer a and pointer b address
    //reg_write32(ACCEL_A, (uint32_t)a);
	//addr = vtop_translate(a);
	addr = (uint64_t)a;
	access_addr(ACCEL_A, OUT, addr, 1);
	int ret1 =access_addr(addr, IN, 0x1, 0);
	int ret2 =access_addr(addr+4, IN, 0x1, 0);
	printf("A: %d %d", ret1, ret2);
	
    //reg_write32(ACCEL_B, (uint32_t)b);
   	//addr = vtop_translate(b);
   	addr =(uint64_t)b;
	access_addr(ACCEL_B, OUT, addr, 1);
	ret1 =access_addr(addr, IN, 0x1, 0);
	ret2 =access_addr(addr+4, IN, 0x1, 0);
	printf("B: %d %d", ret1, ret2);
	
   
    //reg_write32(ACCEL_C, (uint32_t)c);
   	//addr = vtop_translate(c);
	addr =(uint64_t)c;
	access_addr(ACCEL_C, OUT, addr, 1);
 
    //reg_write32(ACCEL_LEN, (uint32_t)length);
	access_addr(ACCEL_LEN, OUT, length, 1);

 
    // Write to ap_start to start the execution 
    //reg_write32(ACCEL_CONTROL, 0x1);
	access_addr(ACCEL_CONTROL, OUT, 0x1, 1);
    //printf("Accel Control: %x\n", reg_read32(ACCEL_CONTROL));

    // Done?
    int done = 0;
    while (!done){
        //done = reg_read32(ACCEL_CONTROL) & AP_DONE_MASK;
	done = access_addr(ACCEL_CONTROL, IN, 0x1, 1) & AP_DONE_MASK;
    }
    return 0;
}

void init_array(uint64_t addr, int length, int init){
	int i;
	for (i = 0; i < length; i ++){
		uint64_t offset = i << 2;
		access_addr(addr+ offset, OUT, init + i, 0);
	}
}

void print_array(uint64_t addr, int length){
	int i;
	for (i = 0; i < length; i ++){
		uint64_t offset = i << 2;
		int value = access_addr(addr+offset, IN, 0, 0);
        	if (i != 0 ) printf(", ");
        	printf("%d", value);
	}
}
int main () {

    int length = LENGTH;
    int i;
    	init_array(0x90000000, length, 0);
    	init_array(0x90000200, length, 5);
//    printf("C = [");
//print_vec(c, length);
//    printf("]\n");

//vadd_accel(a, b, c, length); 
    vadd_accel(0x90000000, 0x90000200, 0x90000400, length);
    //else
//    vadd(a, b, c, length);
//#endif
    printf("A = [");
    //print_vec(a, length);
    print_array(0x90000000, length);
    printf("]\n");

    printf("B = [");
    //print_vec(b, length);
    print_array(0x90000200, length);
    printf("]\n");

    printf("C = [");
    //print_vec(c, length);
    print_array(0x90000400, length);
    printf("]\n");

    return 0;
}
