#include "accel.h"

int vadd(int* length_a, int* b_c) {
    int length = length_a[0];
    for (int i = 0; i < length; i++) {
        b_c[length + i] = length_a[i + 1] + b_c[i];
    }
    return 0;
}
