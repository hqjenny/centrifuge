#ifndef CENTRIFUGE_H
#define CENTRIFUGE_H

#include <stdint.h>
#include <unistd.h>

// Control state for centrifuge. This must be initialized once per tilelink
// accelerator and passed to some centrifuge functions and wrappers.
typedef struct cf_ctl {
  /* GPIO */
  void *gpio; // mmaped physical mem
  uintptr_t gpio_base; // base address for MMIO
} cf_ctl_t;
 
// Represents an allocated centrifuge-compatible buffer
typedef struct cf_buf {
  uint8_t *vaddr;  // mmaped accelerator buffer
  uintptr_t paddr; // physical address of buffer
  size_t size;     // size of mmaped region
} cf_buf_t;

//cf_init initializes centrifuge. This must be called once per tilelink
//accelerator before interacting with other centrifuge utilities or wrappers.
#if defined(CF_LINUX) && defined(CF_ACCEL)
cf_ctl_t cf_init(uintptr_t mmio_base);
#else
// bare-metal applications don't need to register anything (everything is
// direct mapped anyway)
static inline cf_ctl_t cf_init(uintptr_t mmio_base) {
    return ((cf_ctl_t){NULL, mmio_base});
}
#endif //CF_LINUX

#ifdef CF_LINUX
// Allocate memory to be used with centrifuge accelerators. Memory is
// guaranteed to be physically contiguous and pinned. This is only necessary
// (or possible) in LINUX-based applications.
cf_buf_t cf_malloc(size_t size);
void cf_free(cf_buf_t b);
#else
// Convert a statically allocated buffer into a cf_buf_t. This is only valid
// for bare-metal applications (i.e. those with direct mapped virtual->physical
// memory).
static inline cf_buf_t cf_buf_init(uint8_t *buf, size_t len) {
    return (cf_buf_t){buf, (uintptr_t)buf, len};
}
#endif //CF_LINUX

#endif //defined CENTRIFUGE_H
