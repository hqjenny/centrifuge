#pragma once

#include "core/GlobalPtr.hpp"
#include "core/malloc.hpp"
#include "core/alloc.hpp"
#include "core/comm.hpp"

#ifdef SHMEM
  #include "backends/shmem/backend.hpp"
#else
  #include "backends/mpi/backend.hpp"
#endif

#include "core/util.hpp"

namespace BCL {
  uint64_t shared_segment_size;
  void *smem_base_ptr;
}
