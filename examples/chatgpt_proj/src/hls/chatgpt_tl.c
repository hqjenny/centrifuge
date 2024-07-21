#include<stdio.h>
#include<stdint.h>
#include "accel.h"
#define MAX_DATA_SIZE 1024

void print_vec(uint8_t* vec, int length){
    for(int i = 0; i < length; i++){
        if (i != 0 ) printf(", ");
        printf("%d", vec[i]);
    }
}

int main () {

    uint8_t data[MAX_DATA_SIZE], msg[MAX_DATA_SIZE];
    int length = MAX_DATA_SIZE;
    int i;
    for(i = 0; i < length; i++){
      data[i] = i % 128;
    }
    uint64_t begin, end;

    tryDecode(data, length, msg); 

    // tryDecode(const uint8_t *data, int data_size, uint8_t *msg);
    printf("data = [");
    print_vec(data, length);
    printf("]\n");
        
    printf("msg = [");
    print_vec(msg, length);
    printf("]\n");
 
    return 0;
}
