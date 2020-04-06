#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "centrifuge.h"
#include "os_utils.h"

#ifdef CF_LINUX
#include <sys/mman.h>
#endif

#if defined(CF_LINUX) && defined(CF_ACCEL)
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
#endif

#ifdef CF_LINUX
cf_buf_t cf_malloc(size_t size)
{
    cf_buf_t b = (cf_buf_t){0};

    b.size = size;

    //This special file is tied to our contiguous memory allocator in the kernel
    //(custom driver)
    int fd = open("/proc/centrifuge_mmap", O_RDWR|O_SYNC);
    if(fd < 0) {
        printf("cf_malloc: failed to open centrifuge special file\n");
        return (cf_buf_t){0};
    }

    b.vaddr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, fd, 0);
    if(b.vaddr  == NULL) {
        printf("cf_malloc: mmap failure\n");
        return (cf_buf_t){0};
    }
    close(fd);
    
    FILE *vtopf = fopen("/proc/centrifuge_vtop", "rb");
    if(vtopf == NULL) {
        printf("Failed to open /proc/centrifuge_vtop. Is the driver working?");
        return (cf_buf_t){0};
    }

    size_t nread = fread(&(b.paddr), sizeof(uintptr_t), 1, vtopf);
    if(nread != 1) {
        printf("Failed to read from /proc/centrifuge_vtop: %s\n", strerror(errno));
        fclose(vtopf);
        return (cf_buf_t){0};
    }
    fclose(vtopf);

    if(b.paddr == 0) {
        printf("cf_malloc: vtop failure\n");
        return (cf_buf_t){0};
    }

    return b;
}

void cf_free(cf_buf_t b)
{
    munmap(b.vaddr, b.size);
}
#endif
