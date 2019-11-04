
#ifdef CUSTOM_INST 
#include "rocc.h"
#endif
uint64_t  dot_wrapper(uint64_t a, uint64_t b) {
    uint64_t ret_val;

  #ifdef CUSTOM_INST
    #define XCUSTOM_ACC 0
        ROCC_INSTRUCTION_DSS(XCUSTOM_ACC, ret_val, a, b, 0);
  #endif
    return ret_val;
}