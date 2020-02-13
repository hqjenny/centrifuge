#include <sys/mman.h>
#include <fcntl.h>

#include "os_utils.h"

#if 0
// RoCC-accelerated address translation
uint64_t  vtop_translate(uint64_t src) {
  #define XCUSTOM_ACC 3
  //asm volatile ("fence.i");
  uint64_t ret;
  ROCC_INSTRUCTION_DS(XCUSTOM_ACC, ret, src, 0);
  //asm volatile ("fence.i");      
  //printf ("Translate Addr VA %llx to PA %llx\t", src, ret);
  return ret;
}
#else
#define PGMAP_ENTRY_SZ 8
// Generic translation of userspace vaddr to paddr (hard-coded to 4k pages)
uintptr_t vtop(uintptr_t vaddr)
{
    FILE *pgmap = fopen("/proc/self/pagemap", "rb");
    if(pgmap == NULL) {
        return 0;
    }

    int pgsize = 4096;
    off_t off = (vaddr / pgsize) * PGMAP_ENTRY_SZ;
    if(fseek(pgmap, off, SEEK_SET) != 0) {
        fclose(pgmap);
        return 0;
    }

    uintptr_t ent;
    fread(&ent, PGMAP_ENTRY_SZ, 1, pgmap);
    fclose(pgmap);

    //bits 0-54
    uintptr_t pfn = (ent & 0x7FFFFFFFFFFFFF);

    return pfn << 12;
}
#endif

/* int access_addr(unsigned gpio_addr, int direction, int value) */
/* { */
/*  */
/* 	if(fd < 1) { */
/*     fd = open ("/dev/mem", O_RDWR | O_SYNC);  */
/* 	} */
/* 	int c; */
/* 	//int fd; */
/* 	//int direction=IN; */
/* 	//unsigned gpio_addr = 0; */
/* 	//int value = 0; */
/* 	 */
/* 	unsigned page_addr, page_offset; */
/* 	void *ptr; */
/* 	unsigned page_size=sysconf(_SC_PAGESIZE); */
/*  */
/* 	//printf("GPIO access through /dev/mem.\n", page_size); */
/*  */
/* 	if (gpio_addr == 0) { */
/* 		//printf("GPIO physical address is required.\n"); */
/* 		return -1; */
/* 	} */
/* 	 */
/*  */
/* 	#<{(| mmap the device into memory |)}># */
/* 	page_addr = (gpio_addr & (~(page_size-1))); */
/* 	page_offset = gpio_addr - page_addr; */
/* 	ptr = mmap(NULL, page_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, page_addr); */
/*  */
/* 	if (direction == IN) { */
/* 	#<{(| Read value from the device register |)}># */
/* 		value = *((unsigned *)(ptr + page_offset)); */
/* 		//printf("gpio dev-mem test: input: %08x\n",value); */
/* 	} else { */
/* 	#<{(| Write value to the device register |)}># */
/* 		*((unsigned *)(ptr + page_offset)) = value; */
/* 	} */
/* 	munmap(ptr, page_size); */
/*  */
/* 	return value; */
/* } */
