#ifndef VADD_ROCC_H 
#define VADD_ROCC_H

// Calculation is c = a + b where a,b,c are vectors of the same length
// length_a is an array where length_a[0] is the length of all the vectors, and
// length_a[1:] represents vector a.
int vadd_rocc(int* length_a, int* b_c);

static inline uint64_t vadd_rocc_cf_em(uint64_t length_a, uint64_t b_c) {
    return (uint64_t)vadd_rocc((int*)length_a, (int*)b_c);
}
#endif
