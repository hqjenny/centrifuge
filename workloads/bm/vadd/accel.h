#ifndef ACCEL_H
#define ACCEL_H

// MUST ADD
// In generated accel wrapper the ACCEL_WRAPPER is defined
// The accel_wrapper.h is also generated 
#ifdef ACCEL_WRAPPER
#include "accel_wrapper.h" 
#else
int vadd(int* length_a, int* b_c);
#endif 

#endif
