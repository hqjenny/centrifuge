#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "libcf.h"

cf_mem_t *cf_init(uintptr_t gpio_base)
{
  cf_mem_t *mem = malloc(sizeof(cf_mem_t));
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

