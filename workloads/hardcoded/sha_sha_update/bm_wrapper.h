
#ifdef CUSTOM_INST 
#include "rocc.h"
#endif
uint64_t sha_update_wrapper(uint64_t buffer_r, uint64_t sha_info_data) {
  #ifdef CUSTOM_INST
    #define XCUSTOM_ACC 2
        ROCC_INSTRUCTION_SS(XCUSTOM_ACC, buffer_r, sha_info_data, 0);
        ROCC_BARRIER();
  #endif
}
