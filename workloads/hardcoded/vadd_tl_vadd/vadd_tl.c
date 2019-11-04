#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#define LENGTH 80000
#define IN 0
#define OUT 1

#include "rocc.h"
#define ACCEL_CONTROL 0x20000
#define AP_DONE_MASK 0b10

#define ACCEL_INT 0x20004
#define ACCEL_A 0x20018
#define ACCEL_B 0x20024
#define ACCEL_C 0x20030
#define ACCEL_LEN 0x2003c
#define ACCEL_RET 0x20010 
uint64_t read_cycle() {
	uint64_t rd = 0;
	asm volatile("rdcycle %0 " : "=r"(rd));
	return rd;
}	
uint64_t  vtop_translate(uint64_t src){
    #define XCUSTOM_ACC 3
    //asm volatile ("fence.i");
    uint64_t ret;
    ROCC_INSTRUCTION_DS(XCUSTOM_ACC, ret, src, 0);
    //asm volatile ("fence.i");      
    //printf ("Translate Addr VA %llx to PA %llx\t", src, ret);
    return ret;
}

void print_vec(int* vec, int length){
    for(int i = 0; i < length; i++){
        if (i != 0 ) printf(", ");
        printf("%d", vec[i]);
    }
}

int access_addr(unsigned gpio_addr, int direction, int value)
{
	int c;
	int fd;
	//int direction=IN;
	//unsigned gpio_addr = 0;
	//int value = 0;
	
	unsigned page_addr, page_offset;
	void *ptr;
	unsigned page_size=sysconf(_SC_PAGESIZE);

	//printf("GPIO access through /dev/mem.\n", page_size);

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
		//printf("gpio dev-mem test: input: %08x\n",value);
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
	access_addr(ACCEL_INT, OUT, 0);
    // Set up pointer a and pointer b address
    //reg_write32(ACCEL_A, (uint32_t)a);
	addr = vtop_translate(a);
	
	access_addr(ACCEL_A, OUT, addr);
	access_addr(ACCEL_A + 4, OUT, addr >> 32);
	
	//int ret1 =access_addr(addr, IN, 0x1);
	//int ret2 =access_addr(addr+4, IN, 0x1);
	//printf("A: %d %d", ret1, ret2);
	
    //reg_write32(ACCEL_B, (uint32_t)b);
   	addr = vtop_translate(b);
	access_addr(ACCEL_B, OUT, addr);
	access_addr(ACCEL_B + 4, OUT, addr >> 32);
	//ret1 =access_addr(addr, IN, 0x1);
	//ret2 =access_addr(addr+4, IN, 0x1);
	//printf("B: %d %d", ret1, ret2);
	
   
    //reg_write32(ACCEL_C, (uint32_t)c);
   	addr = vtop_translate(c);
	access_addr(ACCEL_C, OUT, addr);
	access_addr(ACCEL_C + 4, OUT, addr >> 32);
 
    //reg_write32(ACCEL_LEN, (uint32_t)length);
	access_addr(ACCEL_LEN, OUT, length);

 
    // Write to ap_start to start the execution 
    //reg_write32(ACCEL_CONTROL, 0x1);
	access_addr(ACCEL_CONTROL, OUT, 0x1);
    //printf("Accel Control: %x\n", reg_read32(ACCEL_CONTROL));

    // Done?
    int done = 0;
    while (!done){
        //done = reg_read32(ACCEL_CONTROL) & AP_DONE_MASK;
	done = access_addr(ACCEL_CONTROL, IN, 0x1) & AP_DONE_MASK;
    }
    return 0;
}

int vadd(int* a, int* b, int* c, int length) {

// For pointer type
#pragma HLS INTERFACE m_axi port=a offset=slave bundle=gmem0 // Direct is for AXI with full 32 bit address space
#pragma HLS INTERFACE m_axi port=b offset=slave bundle=gmem0 // Slave is for AXI4Lite, with burst mode disabled
#pragma HLS INTERFACE m_axi port=c offset=slave bundle=gmem0 

#pragma HLS INTERFACE s_axilite port=a bundle=control
#pragma HLS INTERFACE s_axilite port=b bundle=control
#pragma HLS INTERFACE s_axilite port=c bundle=control
#pragma HLS INTERFACE s_axilite port=length bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

//#pragma HLS DATAFLOW
    int upper = (length >> 3) << 3;
    int i = 0;
    for (i = 0; i < upper; i += 8) {
        // To prevent burst mode
        c[i+0] = a[i+0] +b[i+0];
        c[i+1] = a[i+1] +b[i+1];
        c[i+2] = a[i+2] +b[i+2];
        c[i+3] = a[i+3] +b[i+3];

        c[i+4] = a[i+4] +b[i+4];
        c[i+5] = a[i+5] +b[i+5];
        c[i+6] = a[i+6] +b[i+6];
        c[i+7] = a[i+7] +b[i+7];
    }

    int output = 0;
    for (i = upper; i < length; i++) {
        c[i] = a[i] +b[i];
    }
    return 0;
}

int main () {

    int a[LENGTH], b[LENGTH], c[LENGTH];
    int length = LENGTH;
    int i;
    for(i = 0; i < length; i++){
      a[i] = i;
      b[i] = i + 5;
    }

//    printf("C = [");
//    print_vec(c, length);
//    printf("]\n");
//
uint64_t begin = read_cycle();

#ifdef CUSTOM_DRIVER
    vadd_accel(a, b, c, length); 
    //vadd_accel(0x80000000, 0x80000100, 0x80000200, 8);
#else
    vadd(a, b, c, length);
#endif

uint64_t end = read_cycle();
printf("walltime: %lld\n", end- begin);
printf("last result: %d\n", c[length-1]);
if(0){
    printf("A = [");
    print_vec(a, length);
    printf("]\n");

    printf("B = [");
    print_vec(b, length);
    printf("]\n");

    printf("C = [");
    print_vec(c, length);
    printf("]\n");
}
    return 0;
}
