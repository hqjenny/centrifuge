#include "rocc.h"

#define ACCEL_WRAPPER
#include "accel.h"

#define XCUSTOM_ACC 0

uint64_t vadd(uint64_t length_a, uint64_t b_c)
{
    uint64_t ret_val;

    ROCC_INSTRUCTION_DSS(XCUSTOM_ACC, ret_val, length_a, b_c, 0);
    ROCC_BARRIER();

    return ret_val;
}
