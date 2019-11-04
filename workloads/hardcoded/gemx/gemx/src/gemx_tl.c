#include <stdlib.h> 
#include <stdio.h> 
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "mmio.h"
#define ACCEL_CONTROL 0x20000
#define AP_DONE_MASK 0b10

#define ACCEL_INT 0x20004
#define ACCEL_A 0x20010
#define ACCEL_B 0x2001c
#define IN 0
#define OUT 1

#define MAX_FILE_SIZE 663554

#include "in_buffer.c"

#include "rocc.h"

int fd;
uint64_t buf_size = 0;

int open_mem_device(){
	/* Open /dev/mem file */
	fd = open ("/dev/mem", O_RDWR);
	if (fd < 1) {
		return -1;
	}
	return 0;
}

int close_mem_device(){
	close(fd);
}	

uint64_t read_cycle() {
	uint64_t rd = 0;
	asm volatile("rdcycle %0 " : "=r"(rd));
	return rd;
}	

void* allocate_buffer(unsigned long size){
	void * addr;
	// Set flags to 0x40, mmap returns page_size aligned physical pointer 
	addr = mmap(NULL, size, PROT_READ|PROT_WRITE, 0x40, fd, NULL);
	//ptr = mmap(NULL, page_size, PROT_READ|PROT_WRITE, MAP_ALLOC, fd, NULL);
	printf("0x%llx\n", addr);


	printf("kernel logical addr: 0x%llx\n", addr);
	return addr; 
}

// to is kernel physical addr, from is user virtual addr
int copy_from_user(void* to, void* from, uint64_t size){
	mmap(to, size, PROT_READ|PROT_WRITE, 0x100, fd, (unsigned long)from << 12);
}

// from is kernel physical addr, to is user virtual addr
int copy_to_user(void* to, void* from, uint64_t size){
	mmap(to, size, PROT_READ|PROT_WRITE, 0x200, fd, (unsigned long)from << 12);
}
void* map_to_virt_addr(void* addr) {
	addr = mmap(NULL, buf_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, fd, (unsigned long) addr << 12);
	printf("virtual addr: 0x%llx\n", addr);
	return addr;
}

//Dont free!!
//int free_buffer(void* addr){
//	mmap(addr, buf_size, PROT_READ|PROT_WRITE, 0x80, fd, NULL);
//}

int access_addr(uint64_t gpio_addr, int direction, int value)
{
	int c;
	//int direction=IN;
	//unsigned gpio_addr = 0;
	//int value = 0;
	
	uint64_t page_addr, page_offset;
	void *ptr;
	uint64_t page_size=sysconf(_SC_PAGESIZE);

	printf("GPIO access through /dev/mem.\n", page_size);

	if (gpio_addr == 0) {
		printf("GPIO physical address is required.\n");
		return -1;
	}
	

	/* mmap the device into memory */
	page_addr = (gpio_addr & (~(page_size-1)));
	page_offset = gpio_addr - page_addr;
	ptr = mmap(NULL, page_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, page_addr);

	if (direction == IN) {
	
		value = *((unsigned *)(ptr + page_offset));
		printf("gpio dev-mem test: input: %08x\n",value);
	} else {

		*((unsigned *)(ptr + page_offset)) = value;
	}
	munmap(ptr, page_size);

	return value;
}

uint64_t  vtop_translate(uint64_t src){
    #define XCUSTOM_ACC 3
    asm volatile ("fence.i");
    uint64_t ret;
    ROCC_INSTRUCTION_DS(XCUSTOM_ACC, ret, src, 0);
    asm volatile ("fence.i");      
    printf ("Translate Addr VA %llx to PA %llx\t", src, ret);

	 int status = access_addr(ret, IN, 0);
	 printf("data %x\n", status);
 	status = access_addr(ret+4, IN, 0);
	 printf("data %x\n", status);
	status = access_addr(ret+8, IN, 0);
	 printf("data %x\n", status);
	status = access_addr(ret+12, IN, 0);
	 printf("data %x\n", status);
uint64_t ret0 = ret;
//[B	 ret = ret + MAX_FILE_SIZE - 131074;
//	status = access_addr(ret, IN, 0);
//	 printf("data %x\n", status);
// 	status = access_addr(ret+4, IN, 0);
//	 printf("data %x\n", status);
//	status = access_addr(ret+8, IN, 0);
//	 printf("data %x\n", status);
//	status = access_addr(ret+12, IN, 0);
//	 printf("data %x\n", status);
//

    return ret0;
}


int gemx_accel(char* a, char* b){

	uint64_t addr;
    // Disable interrupt for now
    //reg_write32(ACCEL_INT, 0x0);
	access_addr(ACCEL_INT, OUT, 0);

	 int status = access_addr(ACCEL_CONTROL, IN, 0);
	 printf("Status: %x\n", status);

    // Set up pointer a and pointer b address
    //reg_write32(ACCEL_A, (uint32_t)a);
//	addr = vtop_translate(a);
//	 vtop_translate((uint32_t)a+MAX_FILE_SIZE-131074);
	//return 0;
	addr = (uint64_t)a;
	access_addr(ACCEL_A, OUT, addr);
	access_addr(ACCEL_A + 4, OUT, addr >> 32);
    //reg_write32(ACCEL_B, (uint32_t)b);
 	//addr = vtop_translate(b);
	addr = (uint64_t)b;
	access_addr(ACCEL_B, OUT, addr);
	access_addr(ACCEL_B + 4, OUT, addr >> 32);

  asm volatile ("fence.i");
    // Write to ap_start to start the execution 
    //reg_write32(ACCEL_CONTROL, 0x1);
	access_addr(ACCEL_CONTROL, OUT, 0x1);

    //printf("Accel Control: %x\n", reg_read32(ACCEL_CONTROL));
    	 //status = access_addr(ACCEL_CONTROL, IN, 0);
	 //printf("Status: %x\n", status);


    // Done?
  asm volatile ("fence.i");
    int done = 0;
    while (!done){
        //done = reg_read32(ACCEL_CONTROL) & AP_DONE_MASK;
	done = access_addr(ACCEL_CONTROL, IN, 0x1) & AP_DONE_MASK;
    }

	//addr = vtop_translate((uint32_t)a);
    return 0;
}


int gemx_accel_phy(char* a, char* b){
    // Disable interrupt for now
    reg_write32(ACCEL_INT, 0x0);

    // Set up pointer a and pointer b address
    reg_write32(ACCEL_A, (uint32_t)a);
    reg_write32(ACCEL_B, (uint32_t)b);

    // Write to ap_start to start the execution 
    reg_write32(ACCEL_CONTROL, 0x1);
    //printf("Accel Control: %x\n", reg_read32(ACCEL_CONTROL));

    // Done?
    int done = 0;
    while (!done){
        done = reg_read32(ACCEL_CONTROL) & AP_DONE_MASK;
    }
    return 0;
}




int main (){

	//char in_buffer[MAX_FILE_SIZE];
	//char out_buffer[MAX_FILE_SIZE];
	open_mem_device();
	buf_size = MAX_FILE_SIZE;
	void* virt_addr = allocate_buffer(buf_size);
//	char* virt_addr = (char*) map_to_virt_addr(phy_addr);
	
  char * out_buffer = in_buffer;
     if (mlock((char*) in_buffer, MAX_FILE_SIZE)) {
	  printf("ERROR mlock returns null\n");
	  return 0;
     }
	//mm_mpin((char*) in_buffer, MAX_FILE_SIZE ); 
  
//  printf("char in_buffer[MAX_FILE_SIZE] = {");
//  //for (int i=0; i < MAX_FILE_SIZE; i ++){
//  for (int i=0; i <1; i ++){
//    if ( i != 0)
//      printf(" ,");
//    printf("%d", in_buffer[i]);
//  }
//
//  printf("};\n");
  printf("%x\n", in_buffer);

	uint64_t begin = read_cycle();
  copy_from_user( virt_addr, in_buffer, MAX_FILE_SIZE);
  asm volatile ("fence.i");
  //gemx_accel(phy_addr, phy_addr);
  asm volatile ("fence.i");
  copy_to_user( out_buffer, virt_addr, MAX_FILE_SIZE);
	uint64_t end = read_cycle();
	printf("Time: %lld cycles\n", end - begin);
  printf("char out_buffer[MAX_FILE_SIZE] = {");
  for (int i=MAX_FILE_SIZE - 64; i < MAX_FILE_SIZE; i ++){
    if ( i != 0)
      printf(" ,");
    printf("%d", out_buffer[i]);
  }

  printf("};\n");

  munlock((char*) in_buffer, MAX_FILE_SIZE);
  //writefile(out_path, out_buffer);
  //printf("%s\n", in_buffer);
	//free(buffer);
	return 0;
}
