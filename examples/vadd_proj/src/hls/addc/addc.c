#include<stdio.h>
#include<stdint.h>
#include "accel.h"


int main () {
    
    int a = 4;
    uint64_t begin, end, dur;
    int b = addc(a); 

    printf("%d = %d + 1\n", b, a);

    return 0;
}
