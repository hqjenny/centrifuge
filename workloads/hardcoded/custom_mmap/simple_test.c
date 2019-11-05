#include "mmap_driver.c"


int main (){

    int a[4] = {0, 2, 3, 4};
    int *b; 
    int fd =  mmap_init();
    b = (int *)get_addr(fd);
    b[3] = a[1];
    a[2] = b[2];

    int fd2 =  mmap_init();
    int* c = (int *)get_addr(fd2);
    c[4] = b[5];
    mmap_delete(fd, b);
    mmap_delete(fd2, c);
    return 0;
}    
