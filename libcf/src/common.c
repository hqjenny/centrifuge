#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "libcf.h"

// Control state for centrifuge. This must be initialized once per session and
// passed to some centrifuge functions and wrappers.
typedef struct cf_ctl {
  /* GPIO */
  int fd; // file descriptor mapping /dev/mem
  void *gpio; // mmaped physical mem
  uintptr_t gpio_base; // base address for MMIO

} cf_ctl_t;

typedef struct cf_buf {
  uint8_t *a_buf; // mmaped accelerator buffer
  size_t a_buf_sz; // size of mmaped region
}

#ifdef CF_TILELINK
cf_mem_t *cf_init(uintptr_t cf_init_param);
#else
#define cf_init(A) NULL
#endif

#ifdef CF_TILELINK
void *cf_malloc(size_t size);
cf_free(kkk
#else
#define cf_malloc malloc
#endif
