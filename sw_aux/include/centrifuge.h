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

#if defined(CF_LINUX) && defined(CF_ACCEL)
//cf_init initializes centrifuge. This must be called once per tilelink
//accelerator before interacting with other centrifuge utilities or wrappers.
cf_ctl_t cf_init(uintptr_t mmio_base);

// Allocate memory to be used with centrifuge accelerators. Memory is
// guaranteed to be physically contiguous and pinned. This is only necessary
// (or possible) in LINUX-based applications.
cf_buf_t cf_malloc(size_t size);
void cf_free(cf_buf_t b);

#elif defined(CF_LINUX) && !defined(CF_ACCEL)
//Non-accelerated linux applications fall back to normal system services as
//much as possible while preserving libcf semantics
static inline cf_ctl_t cf_init(uintptr_t mmio_base) {
    return ((cf_ctl_t){NULL, mmio_base});
}

#define cf_malloc(size) (cf_buf_t){malloc(size), 0, size}
#define cf_free(b) free(b.vaddr)

// bare-metal applications don't need to register anything (everything is
// direct mapped anyway). They also cannot call cf_malloc, and instead use
// cf_buf_init (which only works bare metal).
#else
static inline cf_ctl_t cf_init(uintptr_t mmio_base) {
    return ((cf_ctl_t){NULL, mmio_base});
}

// Convert a statically allocated buffer into a cf_buf_t. This is only valid
// for bare-metal applications (i.e. those with direct mapped virtual->physical
// memory).
static inline cf_buf_t cf_buf_init(uint8_t *buf, size_t len) {
    return (cf_buf_t){buf, (uintptr_t)buf, len};
}
#endif

#endif //defined CENTRIFUGE_H
