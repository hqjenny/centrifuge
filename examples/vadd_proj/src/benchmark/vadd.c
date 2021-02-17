#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include <centrifuge.h>

#ifdef CF_ACCEL
#include "tl0_vadd_tl_vadd_tl_wrapper.h"
#include "rocc0_vadd_rocc_vadd_rocc_wrapper.h"
#else
#include "vadd_rocc.h"
#include "vadd_tl.h"
#endif

#define LENGTH 80
void print_vec(int* vec, int length){
    for(int i = 0; i < length; i++){
        if (i != 0 ) printf(", ");
        printf("%d", vec[i]);
    }
}

// Check whether two vectors are equal. Returns the first index at
// which they differ, or -1 if they are identical.
int compare_vec(int *a, int *b, int length) {
  for(int i = 0; i < length; i++) {
    if(a[i] != b[i]) {
      return i;
    }
  }
  return -1;
}

int test_rocc(int *c, int *a, int *b, int len) {
    int length_a[len + 1], b_c[len + len];
    length_a[0] = len;

    // Copy inputs into rocc-formatted arrays
    for(int i = 0; i < len; i++){
      length_a[i + 1] = a[i];
      b_c[i] = b[i];
    }

    uint64_t begin, end, dur;
#ifdef CF_ACCEL
    vadd_rocc_cf_accel((uint64_t)length_a, (uint64_t)b_c); 
#else
    vadd_rocc_em((uint64_t)length_a, (uint64_t)b_c); 
#endif

    // Get output into form expected by test
    for(int i = 0; i < len; i++){
      c[i] = b_c[len + i];
    }

    return 0;
}

int test_tl(cf_ctl_t *ctl, cf_buf_t *c, cf_buf_t *a, cf_buf_t *b, int len)
{
    /* NOTE: libcf integration with wrapper generation doesn't work quite yet
     * so we are falling back to a more direct approach (that will only work on
     * bare metal in accelerated mode. */
#ifdef CF_ACCEL
    /* vadd_tl_cf_accel(ctl, a, b, c, len); */
    vadd_tl_cf_accel((uint64_t)a->vaddr, (uint64_t)b, (uint64_t)c, len);
#else
    /* vadd_tl_cf_em(ctl, a, b, c, len); */
    vadd_tl_cf_em((uint64_t)a->vaddr, (uint64_t)b, (uint64_t)c, len);
#endif
  return 0;
}

int main(int argc, char *argv[])
{
    cf_buf_t a, b, tl_out;

    /*=========================================================================
     * MEMORY ALLOCATION
    *=======================================================================*/
#ifdef CF_LINUX
    // Dynamically allocate memory that is compatible with tilelink.
    a = cf_malloc(LENGTH*sizeof(int));
    b = cf_malloc(LENGTH*sizeof(int));
    tl_out = cf_malloc(LENGTH*sizeof(int));
#else
    //Allocate bare-metal buffers on the stack. This only works with
    //direct-mapped memory (no virtual memory).
    int abuf[LENGTH], bbuf[LENGTH];
    int tl_out_buf[LENGTH] = {0};

    a = cf_buf_init((uint8_t*)abuf, LENGTH);
    b = cf_buf_init((uint8_t*)bbuf, LENGTH);
    tl_out = cf_buf_init((uint8_t*)tl_out_buf, LENGTH);
#endif //CF_LINUX

    /*=========================================================================
     * DATA INITIALIZATION
     *=======================================================================*/
    // rocc-only memory can be allocated however you please, it respects
    // virtual memory and has no additional restrictions.
    int rocc_out[LENGTH] = {0};
    int golden_out[LENGTH] = {0};

    for(int i = 0; i < LENGTH; i++){
      ((int*)(a.vaddr))[i] = i;
      ((int*)(b.vaddr))[i] = i + 5;
    }

    for(int i = 0; i < LENGTH; i++) {
      golden_out[i] = ((int*)(a.vaddr))[i] + ((int*)(b.vaddr))[i];
    }

    /*=========================================================================
     * RoCC TEST
     *=======================================================================*/
    // RoCC only needs the virtual address
    test_rocc(rocc_out, (int*)a.vaddr, (int*)b.vaddr, LENGTH);
    if(compare_vec(rocc_out, golden_out, LENGTH) != -1) {
      printf("Test Failure: rocc output does not match\n");
      printf("Expected:\n");
      print_vec(golden_out, LENGTH);
      printf("\nGot:\n");
      print_vec(rocc_out, LENGTH);
      return EXIT_FAILURE;
    }
      
    /*=========================================================================
     * TL TEST
     *=======================================================================*/
    //Initialize the tilelink accelerator, this only needs to be done once per
    //session and can be re-used for multiple invocations. The CF_VADD_TL_BASE
    //macro is defined in the generated wrapper, we also manually define it in
    //our baseline code so that we can easily switch between the two
    //implementations.
    cf_ctl_t vadd_ctl = cf_init(CF_VADD_TL_BASE);

    test_tl(&vadd_ctl, &tl_out, &a, &b, LENGTH);
    if(compare_vec((int*)(tl_out.vaddr), golden_out, LENGTH) != -1) {
      printf("Test Failure: tilelink output does not match\n");
      printf("Expected:\n");
      print_vec(golden_out, LENGTH);
      printf("\nGot:\n");
      print_vec((int*)(tl_out.vaddr), LENGTH);
      printf("\n");
      return EXIT_FAILURE;
    }

    printf("Test Success!\n");
    return EXIT_SUCCESS;
}
