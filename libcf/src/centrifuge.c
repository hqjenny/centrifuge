#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "centrifuge.h"
#include "os_utils.h"

cf_ctl_t cf_init(uintptr_t gpio_base)
{
    cf_ctl_t ctl;
    ctl.gpio_base = gpio_base;

    /* /dev/mem lets us access physical memory */
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd < 1) {
        return (cf_ctl_t){0};
    }
  
    /* We assume that device MMIO is page aligned and fits in one page */
    unsigned page_size = sysconf(_SC_PAGESIZE);
    uintptr_t page_addr = (ctl.gpio_base & (~(page_size-1)));
    ctl.gpio = mmap(NULL, page_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, gpio_base);
    if(ctl.gpio == NULL) {
        close(fd);
        return (cf_ctl_t){0};
    }

    close(fd);
    return ctl;
}

cf_buf_t cf_malloc(size_t size)
{
    cf_buf_t b;

    b.size = size;

    //This special file is tied to our contiguous memory allocator in the kernel
    //(custom driver)
    int fd = open("/proc/centrifuge_mmap", O_RDWR|O_SYNC);
    if(fd < 0) {
        return (cf_buf_t){0};
    }

    b.vaddr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(b.vaddr  == NULL) {
        return (cf_buf_t){0};
    }
    close(fd);
    
    b.paddr = vtop((uintptr_t)b.vaddr);
    if(b.paddr == 0) {
        return (cf_buf_t){0};
    }

    return b;
}

void cf_free(cf_buf_t b)
{
    munmap(b.vaddr, b.size);
}
