#pragma once

#include <mpi.h>

#include "progress_thread.hpp"
#include "rpc.hpp"
#include "alloc.hpp"
#include "comm.hpp"
#include "ops.hpp"
#include "future.hpp"

namespace BCL {
  extern uint64_t shared_segment_size;
  extern void *smem_base_ptr;

  extern void init_malloc();

  MPI_Comm comm;
  MPI_Win win;
  MPI_Info info;

  bool we_initialized;
  bool bcl_finalized;

  uint64_t my_rank;
  uint64_t my_nprocs;

  extern void start_progress_thread();
  extern void stop_progress_thread();

  uint64_t rank() {
    return BCL::my_rank;
  }

  uint64_t nprocs() {
    return BCL::my_nprocs;
  }

  bool mpi_finalized() {
    int flag;
    MPI_Finalized(&flag);
    return flag;
  }

  bool mpi_initialized() {
    int flag;
    MPI_Initialized(&flag);
    return flag;
  }

  void barrier() {
    MPI_Win_unlock_all(win);
    MPI_Barrier(BCL::comm);
    MPI_Win_lock_all(0, win);
  }

  void init(void* ptr, size_t size, bool progress = false) {
    BCL::comm = MPI_COMM_WORLD;
    BCL::shared_segment_size = size;

    BCL::smem_base_ptr = ptr;

    if (!mpi_initialized()) {
      if (!progress) {
        MPI_Init(NULL, NULL);
      } else {
        int provided;
        MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);
        if (provided < MPI_THREAD_MULTIPLE) {
          fprintf(stderr, "warning: MPI runtime provided thread support level %d < %d requested\n", provided, MPI_THREAD_MULTIPLE);
        }
      }
      we_initialized = true;
    }

    int rank, nprocs;
    MPI_Comm_rank(BCL::comm, &rank);
    MPI_Comm_size(BCL::comm, &nprocs);
    BCL::my_rank = rank;
    BCL::my_nprocs = nprocs;

    MPI_Info_create(&info);
    MPI_Info_set(info, "accumulate_ordering", "none");
    MPI_Info_set(info, "accumulate_ops", "same_op_no_op");
    MPI_Info_set(info, "same_size", "true");
    MPI_Info_set(info, "same_disp_unit", "true");

    MPI_Win_create(BCL::smem_base_ptr, size, 1, info, BCL::comm, &win);

    bcl_finalized = false;

    init_malloc();

    MPI_Barrier(BCL::comm);
    MPI_Win_lock_all(0, win);
    if (progress) {
      BCL::start_progress_thread();
    }
    BCL::barrier();
  }

  // MPI communicator, shared_segment_size in MB,
  // and whether to start the progress thread.
  void init(uint64_t shared_segment_size = 256, bool progress = false) {
    BCL::comm = MPI_COMM_WORLD;
    BCL::shared_segment_size = 1024*1024*shared_segment_size;

    if (!mpi_initialized()) {
      if (!progress) {
        MPI_Init(NULL, NULL);
      } else {
        int provided;
        MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);
        if (provided < MPI_THREAD_MULTIPLE) {
          fprintf(stderr, "warning: MPI runtime provided thread support level %d < %d requested\n", provided, MPI_THREAD_MULTIPLE);
        }
      }
      we_initialized = true;
    }

    int rank, nprocs;
    MPI_Comm_rank(BCL::comm, &rank);
    MPI_Comm_size(BCL::comm, &nprocs);
    BCL::my_rank = rank;
    BCL::my_nprocs = nprocs;

    MPI_Info_create(&info);
    MPI_Info_set(info, "accumulate_ordering", "none");
    MPI_Info_set(info, "accumulate_ops", "same_op_no_op");
    MPI_Info_set(info, "same_size", "true");
    MPI_Info_set(info, "same_disp_unit", "true");

    MPI_Win_allocate(BCL::shared_segment_size, 1, info, BCL::comm,
      &smem_base_ptr, &win);

    bcl_finalized = false;

    init_malloc();

    MPI_Barrier(BCL::comm);
    MPI_Win_lock_all(0, win);
    if (progress) {
      BCL::start_progress_thread();
    }
    BCL::barrier();
  }

  void finalize() {
    BCL::barrier();
    BCL::stop_progress_thread();
    MPI_Win_unlock_all(win);
    MPI_Info_free(&info);
    // TODO: comment this line out if you have bugs.
    MPI_Win_free(&win);
    if (we_initialized && !mpi_finalized()) {
      MPI_Finalize();
    }
    bcl_finalized = true;
  }
}
