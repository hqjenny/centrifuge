#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	int c;
	int fd;
	unsigned gpio_addr = 0;
	int value = 0;
	
	unsigned long page_addr, page_offset;
	void *ptr;
	unsigned page_size=sysconf(_SC_PAGESIZE);

	printf("GPIO access through /dev/mem.\n", page_size);

	/* Open /dev/mem file */
	fd = open ("/dev/mem", O_RDWR);
	if (fd < 1) {
		perror(argv[0]);
		return -1;
	}

	/* mmap the device into memory */
	page_addr = (gpio_addr & (~(page_size-1)));
	page_offset = gpio_addr - page_addr;
	ptr = mmap(NULL, page_size, PROT_READ|PROT_WRITE, 0x40, fd, page_addr);
	printf("0x%llx\n", ptr);

	page_addr = ptr;
	ptr = mmap(NULL, 1 , PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, fd, page_addr);

	value = *((unsigned *)ptr);
	printf("gpio dev-mem test: input: %08x\n",value);
	*((unsigned *)(ptr + 4)) = 77;
	value = *((unsigned *)(ptr + 4));
	printf("gpio dev-mem test: input: %08x\n",value);

	ptr = mmap(ptr, page_size, PROT_READ|PROT_WRITE, 0x80, fd, page_addr);
	return 0;
}


