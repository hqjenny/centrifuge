#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* uintmax_t */
#include <string.h>
#include <sys/mman.h>
#include <unistd.h> /* sysconf */
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12

//static int fd;
int mmap_init()
{
    char* file = "/proc/hls_alloc_static";
    int fd = open(file, O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open");
			  printf("cannot open file /proc/hls_alloc\n");
        assert(0);
    }
    //printf("fd = %d\n", fd);
    return fd;
}

unsigned long get_order(unsigned long size){
		unsigned long num_pages = (size  - 1 )>> PAGE_SHIFT;	
    unsigned order = 0;
		int i;	
		for (i = 0; i < 32; i++){
			if(!num_pages) {
				order = i;
				break;
			}
			num_pages	= num_pages >> 1; 
		}
    return order;
}


unsigned long hls_kalloc(int fd, unsigned long size){
    void *addr;
    unsigned long page_order = get_order(size);
    printf("page_size %d, page_order %d \n", size, page_order);
    unsigned long page_aligned_size = ((1 << page_order) * PAGE_SIZE);
    //puts("mmap");
    //addr = mmap(NULL, page_aligned_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(fd);
	  addr = mmap(NULL, page_aligned_size, PROT_READ|PROT_WRITE,
			MAP_POPULATE | MAP_PRIVATE, fd, 0);

	if (addr == MAP_FAILED) {
		perror("mmap");
		printf("MAP_FAILED : %s", addr);
		close(fd);
		return -1;
	}

    return (unsigned long)addr;
 } 

int hls_kfree(int fd, unsigned long  addr, unsigned long size) {
    unsigned long page_order = get_order(size);
    unsigned long page_aligned_size = ((1 << page_order) * PAGE_SIZE);
    if (munmap((void*)addr, page_aligned_size)) {
        perror("munmap");
        assert(0);
    }
    close(fd);
    return 0;
}


void copy_to_buffer(int fd, char* target_addr, char* src_addr, unsigned size){
    memcpy(target_addr, src_addr, size);
    //return target_addr;
}


