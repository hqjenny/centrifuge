
#ifdef CUSTOM_INST 
#include "rocc.h"
#endif
uint64_t  encode_wrapper(uint64_t xin1, uint64_t xin2) {
    uint64_t ret_val;

  #ifdef CUSTOM_INST
    #define XCUSTOM_ACC 1
        ROCC_INSTRUCTION_DSS(XCUSTOM_ACC, ret_val, xin1, xin2, 0);
  #endif
    return ret_val;
}