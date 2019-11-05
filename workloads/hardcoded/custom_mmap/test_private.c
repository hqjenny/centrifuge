#define _XOPEN_SOURCE 700
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* uintmax_t */
#include <string.h>
#include <sys/mman.h>
#include <unistd.h> /* sysconf */

#include "common.h" /* virt_to_phys_user */

#define MAP_POPULATE 0x8000
enum { BUFFER_SIZE = 4 };

int main(int argc, char **argv)
{
    int fd;
    long page_size;
    char *address1, *address2;
    char buf[BUFFER_SIZE];
    uintptr_t paddr;

//    if (argc < 2) {
//        printf("Usage: %s <mmap_file>\n", argv[0]);
//        return EXIT_FAILURE;
//    }
//

    argv[1] = "/proc/custom_mmap";
    page_size = sysconf(_SC_PAGE_SIZE) * 128;
    printf("open pathname = %s of size %d\n", argv[1], page_size);

    fd = open(argv[1], O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open");
        assert(0);
    }
    printf("fd = %d\n", fd);

    /* mmap twice for double fun. */
    puts("mmap 1");
    //address1 = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    //address1 = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    address1 = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (address1 == MAP_FAILED) {
        perror("mmap");
        assert(0);
    }
    
    printf("address1 %lx\n", address1);

    /* Read and modify memory. */
    puts("access 1");
    //assert(!strcmp(address1, "asdf"));

    memset(address1, 7, page_size);
    unsigned long offset = 4500;
    /* vm_fault */

    printf("address1 + offset %lx\n", address1 + offset);
    strcpy(address1 + offset, "qwer");

    /* Check that the physical addresses are the same.
     * They are, but TODO why virt_to_phys on kernel gives a different value? */

    /* Check that modifications made from userland are also visible from the kernel. */
  //  int ret  = read(fd, buf, BUFFER_SIZE);
  //  printf("ret %d\n", ret);
 //   assert(!memcmp(buf, "qwer", BUFFER_SIZE));

    /* Modify the data from the kernel, and check that the change is visible from userland. */
  //  write(fd, "zxcv", 4);
  //  assert(!strcmp(address1+offset, "zxcv"));

    /* Cleanup. */
    puts("munmap 1");
    if (munmap(address1, page_size)) {
        perror("munmap");
        assert(0);
    }

    puts("close");
    close(fd);
    return EXIT_SUCCESS;
}

