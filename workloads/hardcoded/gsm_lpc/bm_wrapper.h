#ifdef CUSTOM_INST 
#include "rocc.h"
#endif

void lpc_wrapper(uint64_t s, uint64_t LARc) {

  #ifdef CUSTOM_INST
    #define XCUSTOM_ACC 1
        ROCC_INSTRUCTION_SS(XCUSTOM_ACC, s, LARc, 0);
        ROCC_BARRIER();
  #endif
}
