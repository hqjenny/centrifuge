#ifndef VADD_TL_H
#define VADD_TL_H

#include <centrifuge.h>

#ifndef CF_VADD_TL_BASE
// If this is wrapping the baseline (non-acclerated) version, we fill in a
// dummy base address.
#define CF_VADD_TL_BASE 0
#endif

// This is the HLS function we will accelerate with centrifuge
// Vector add: c = a + b
int vadd_tl(int * a, int * b, int* c, int length);

// centrifuge-compatible wrapper of our orignal (baseline) function. The format
// of this wrapper must match the wrapper generated by centrifuge (see
// centrifuge documentation for how this signiture is determined).
// This allows our benchmark code to easily switch between the accelerated and
// baseline implementations.
// NOTE: this is not required by centrifuge, it is purely for the convenience
// of our benchmark application.
static inline int vadd_tl_cf_em(cf_ctl_t *ctl, cf_buf_t *a, cf_buf_t *b, cf_buf_t *c, int length) {
    return vadd_tl((int*)a->vaddr, (int*)b->vaddr, (int*)c->vaddr, length);
}

#endif