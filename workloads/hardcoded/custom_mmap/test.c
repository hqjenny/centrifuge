#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdint.h>
#include <assert.h>
#include "common.h"

#define ORDER 8
#define PAGE_SIZE 4096
#define MEMSZ (PAGE_SIZE * (1 << ORDER))

int main(int argc, char *argv[])
{
  uintptr_t paddr;
	char *vaddr;
	
	/*memory map*/
	int map_fd = open("/proc/test2",  O_RDWR|O_SYNC);
	if (map_fd < 0) {
			printf("cannot open file /proc/test2\n");
			return -1;
	}
	vaddr = mmap(NULL, MEMSZ, PROT_READ|PROT_WRITE,
			MAP_SHARED, map_fd, 0);

	if (vaddr == MAP_FAILED) {
		perror("mmap");
		printf("MAP_FAILED : %s", vaddr);
		close(map_fd);
		return -1;
	}

  char *buf;
  for(int i = 0; i < MEMSZ; i += PAGE_SIZE) {
    int val;
    buf = vaddr + i;
    printf("vaddr: %p \n", buf);
    assert(!virt_to_phys_user(&paddr, getpid(), (uintptr_t)buf));
    printf("paddr = 0x%jx\n", (uintmax_t)paddr);
    val = atoi(buf);
    printf("val: %d\n", val);
    sprintf(buf, "%d", -val);
  }

	int ret = munmap(vaddr, PAGE_SIZE*2);
	if (ret) {
			printf("munmap failed:%d \n",ret);
	}
	close(map_fd);
	return 0;
}
