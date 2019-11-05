#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* uintmax_t */
#include <string.h>
#include <sys/mman.h>
#include <unistd.h> /* sysconf */

#include "common.h" /* virt_to_phys_user */
#define PAGE_SIZE 4096

int mmap_init()
{
    int fd;
    char * file = "/proc/origin_mmap";
    //page_size = sysconf(_SC_PAGE_SIZE);
    unsigned long page_size =PAGE_SIZE; 
    //printf("page_size %d\n", page_size);
    //printf("open pathname = %s\n", file);
    fd = open(file, O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open");
        assert(0);
    }
    //printf("fd = %d\n", fd);
    return fd;
}

unsigned long get_addr(int fd){
    unsigned long page_size =PAGE_SIZE; 
    //printf("page_size %d\n", page_size);
    char *addr;

    //puts("mmap");
    addr = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    return addr;
 } 

int mmap_delete(int fd, unsigned long  addr) {
    unsigned long page_size =PAGE_SIZE; 
    if (munmap(addr, page_size)) {
        perror("munmap");
        assert(0);
    }
    close(fd);
}

char* copy_to_buffer(char* addr, unsigned length, int fd){
    char * target_addr  = (void *)get_addr(fd);
    memcpy(target_addr, addr, length);
    //for (i  = 0; i < length; i++){
    //    target_addr[i] = 
    //} 
    return target_addr;
}


