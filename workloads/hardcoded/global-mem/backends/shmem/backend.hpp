#pragma once

#include <cassert>

#include <mpp/shmem.h>

#include "alloc.hpp"
#include "comm.hpp"
#include "ops.hpp"
#include "future.hpp"

namespace BCL {
  extern uint64_t shared_segment_size;
  extern void *smem_base_ptr;

  extern void init_malloc();

  bool we_initialized;
  bool bcl_finalized;

  uint64_t my_rank;
  uint64_t my_nprocs;

  uint64_t rank() {
    return BCL::my_rank;
  }

  uint64_t nprocs() {
    return BCL::my_nprocs;
  }

  void barrier() {
    shmem_barrier_all();
  }

  // MPI communicator, shared_segment_size in MB,
  // and whether to start the progress thread.
  void init(uint64_t shared_segment_size = 256, bool progress = false) {
    BCL::shared_segment_size = 1024*1024*shared_segment_size;

    assert(progress == false);

    shmem_init();

    BCL::my_rank = shmem_my_pe();
    BCL::my_nprocs = shmem_n_pes();

    BCL::smem_base_ptr = shmem_malloc(BCL::shared_segment_size);

    if (BCL::smem_base_ptr == NULL) {
      throw std::runtime_error("BCL: Could not allocate shared memory segment.");
    }

    bcl_finalized = false;

    init_malloc();

    BCL::barrier();
  }

  void finalize() {
    BCL::barrier();
    shmem_free(smem_base_ptr);
    shmem_finalize();

    bcl_finalized = true;
  }
}
