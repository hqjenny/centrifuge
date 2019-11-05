
#ifdef CUSTOM_INST 
#include "rocc.h"
#endif
uint64_t  decode_wrapper(uint64_t input_r) {
    uint64_t ret_val;

  #ifdef CUSTOM_INST
    #define XCUSTOM_ACC 2
        ROCC_INSTRUCTION_DS(XCUSTOM_ACC, ret_val, input_r, 0);
  #endif
    return ret_val;
}