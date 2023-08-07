#ifndef ACCEL_H
#define ACCEL_H

#include <stdint.h>
// MUST ADD
// In generated accel wrapper the ACCEL_WRAPPER is defined
// The accel_wrapper.h is also generated 
#ifdef ACCEL_WRAPPER
#include "accel_wrapper.h" 
#else
//int vadd(int * a, int * b, int* c, int length);
int tryDecode(const uint8_t *data, int data_size, uint8_t *msg);
#endif 

#endif
