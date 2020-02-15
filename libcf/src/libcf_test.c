#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "centrifuge.h"
#include "os_utils.h"

bool check_cf_malloc(size_t size)
{
    cf_buf_t b = cf_malloc(size);

    if(memcmp(&b, &(cf_buf_t){0}, sizeof(cf_buf_t)) == 0) {
        printf("Buffer allocation failed\n");
        return false;
    }

    if(b.size != size) {
        printf("Buffer size does not match expected size:\n");
        printf("\tExpected: %ul\tGot: %ul\n", size, b.size);
        return false;
    }

    /* This test doesn't work unless vtop can handle cf_malloc addresses */
    /* uintptr_t paddr = vtop((uintptr_t)b.vaddr); */
    /* if(b.paddr != vtop((uintptr_t)b.vaddr)) { */
    /*     printf("Recorded paddr does not match actual paddr:\n"); */
    /*     printf("\tExpected: %ul\tGot: %ul\n", paddr, b.paddr); */
    /*     return false; */
    /* } */

    // Page alignment
    if(b.paddr % getpagesize() != 0) {
        printf("Physical buffer not page aligned: b.paddr == %ul\n", b.paddr);
        return false;
    }

    // Contiguity test (assumes b.paddr is page aligned)
    /* for(int pgoff = 0; pgoff < b.size; pgoff += getpagesize()) { */
    /*     paddr = vtop((uintptr_t)b.vaddr + pgoff); */
    /*     if(paddr != b.paddr + pgoff) { */
    /*         printf("Physical pages not contiguous:\n"); */
    /*         printf("First Paddr: %ul\tPage %d paddr: %ul\n", b.paddr, pgoff / getpagesize(), paddr); */
    /*         return false; */
    /*     } */
    /* } */

    return true;
}

bool test_cf_malloc(void)
{
    cf_buf_t b;
    uintptr_t paddr;

    /* #<{(| Small allocation |)}># */
    /* if(!check_cf_malloc(128)) { */
    /*     printf("128B cf_malloc test failure\n"); */
    /*     return false; */
    /* } */
    /*  */
    /* #<{(| Exactly 1 page |)}># */
    /* if(!check_cf_malloc(getpagesize())) { */
    /*     printf("1 page cf_malloc test failure\n"); */
    /*     return false; */
    /* } */
    /*  */
    /* Small number of pages */
    if(!check_cf_malloc(5*getpagesize())) {
        printf("5 page cf_malloc test failure\n");
        return false;
    }
    /*  */
    /* #<{(| 1 MB |)}># */
    /* if(!check_cf_malloc(1024*1024)) { */
    /*     printf("1MiB cf_malloc test failure\n"); */
    /*     return false; */
    /* } */
    /*  */
    /* #<{(| Max Size |)}># */
    /* if(!check_cf_malloc(65*1024*1024)) { */
    /*     printf("Max (64MiB) cf_malloc test failure\n"); */
    /*     return false; */
    /* } */
    /*  */
    return true;
}

bool test_vtop(void) {
    uint8_t *b = malloc(1);
    if(b == NULL) {
        printf("failed to allocate test buffer\n");
        return false;
    }

    uintptr_t paddr = vtop((uintptr_t)b);
    if(paddr == 0) {
        printf("Failed to translate paddr\n");
        return false;
    }

    printf("vtop test: 0x%lx translated to physical 0x%lx\n", (uintptr_t)b, paddr);
    return true;
}

int main(int argc, char *argv[])
{
    if(!test_vtop()) {
        printf("vtop test failure\n");
        return EXIT_FAILURE;
    }
    printf("vtop test success\n");

    if(!test_cf_malloc()) {
        printf("cf_malloc test failure\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
