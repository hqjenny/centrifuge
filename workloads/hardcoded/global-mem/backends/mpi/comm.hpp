#pragma once

#include <stdexcept>
#include <cstring>

#include "backend.hpp"
#include "ops.hpp"
#include "future.hpp"

namespace BCL {
  extern MPI_Comm comm;
  extern MPI_Win win;

  template <typename T>
  T broadcast(T &val, uint64_t root) {
    MPI_Bcast(&val, sizeof(T), MPI_CHAR, root, BCL::comm);
    return val;
  }

  template <typename T>
  T allreduce(const T &val, const abstract_op <T> &op) {
    T rv;
    MPI_Allreduce(&val, &rv, 1, op.type(), op.op(), BCL::comm);
    return rv;
  }

  template <typename T>
  future<BCL::GlobalPtr<T>> arget(const GlobalPtr<T>& src, size_t size) {
    BCL::GlobalPtr<T> dst = BCL::alloc<T>(size);
    if (dst == nullptr) {
      throw std::runtime_error("BCL::arget ran out of memory for receiving tile");
    }
    MPI_Request request;
    MPI_Rget(dst.local(), size*sizeof(T), MPI_CHAR, src.rank, src.ptr, size*sizeof(T),
             MPI_CHAR, BCL::win, &request);
    return future<BCL::GlobalPtr<T>>(std::move(dst), request);
  }

  // Read size T's from src -> dst
  // Blocks until dst is ready.
  template <typename T>
  void read(const GlobalPtr <T> &src, T *dst, const size_t size) {
    if (src.rank > BCL::nprocs()) {
      throw std::runtime_error("BCL read(): request to read from rank " +
        std::to_string(src.rank) + ", which does not exist");
    }
    if (src.rank == BCL::rank()) {
      memcpy(dst, src.local(), size*sizeof(T));
    } else {
      MPI_Request request;
      MPI_Rget(dst, size*sizeof(T), MPI_CHAR, src.rank, src.ptr, size*sizeof(T),
        MPI_CHAR, BCL::win, &request);
      MPI_Wait(&request, MPI_STATUS_IGNORE);
    }
  }

  // Read size T's from src -> dst
  // Blocks until dst is ready.
  template <typename T>
  void atomic_read(const GlobalPtr <T> &src, T *dst, const size_t size) {
    if (src.rank > BCL::nprocs()) {
      throw std::runtime_error("BCL read(): request to read from rank " +
        std::to_string(src.rank) + ", which does not exist");
    }
    MPI_Request request;
    MPI_Rget(dst, size*sizeof(T), MPI_CHAR, src.rank, src.ptr, size*sizeof(T),
      MPI_CHAR, BCL::win, &request);
    MPI_Wait(&request, MPI_STATUS_IGNORE);
  }

  // Write size T's from src -> dst
  // Returns after src buffer is sent
  // and okay to be modified; no
  // guarantee that memop is complete
  // until BCL::barrier()
  template <typename T>
  void write(const T *src, const GlobalPtr <T> &dst, const size_t size) {
    if (dst.rank > BCL::nprocs()) {
      throw std::runtime_error("BCL write(): request to write to rank " +
        std::to_string(dst.rank) + ", which does not exist");
    }
    if (dst.rank == BCL::rank()) {
      memcpy(dst.local(), src, size*sizeof(T));
    } else {
      MPI_Request request;
      MPI_Rput(src, size*sizeof(T), MPI_CHAR, dst.rank, dst.ptr, size*sizeof(T),
        MPI_CHAR, BCL::win, &request);
      MPI_Wait(&request, MPI_STATUS_IGNORE);
    }
  }

  template <typename T>
  T fetch_and_op(const GlobalPtr <T> ptr, const T &val, const atomic_op <T> &op) {
    T rv;
    MPI_Request request;
    MPI_Rget_accumulate(&val, 1, op.type(), &rv, 1, op.type(), ptr.rank, ptr.ptr,
      1, op.type(), op.op(), BCL::win, &request);
    MPI_Wait(&request, MPI_STATUS_IGNORE);
    return rv;
  }

  // MPI one-sided is basically terrible :/
  // TODO: beter CAS syntax
  int int_compare_and_swap(const GlobalPtr <int> ptr, const int old_val,
    const int new_val) {
    int result;
    MPI_Compare_and_swap(&new_val, &old_val, &result, MPI_INT, ptr.rank, ptr.ptr, BCL::win);
    MPI_Win_flush_local(ptr.rank, BCL::win);
    return result;
  }

  uint16_t uint16_compare_and_swap(const GlobalPtr <uint16_t> ptr, const uint16_t old_val, const uint16_t new_val) {
    uint16_t result;
    MPI_Compare_and_swap(&new_val, &old_val, &result, MPI_UNSIGNED_SHORT, ptr.rank, ptr.ptr, BCL::win);
    MPI_Win_flush_local(ptr.rank, BCL::win);
    return result;
  }

  uint64_t uint64_compare_and_swap(const GlobalPtr <uint64_t> ptr, const uint64_t old_val, const uint64_t new_val) {
    uint64_t result;
    MPI_Compare_and_swap(&new_val, &old_val, &result, MPI_UNSIGNED_LONG_LONG, ptr.rank, ptr.ptr, BCL::win);
    MPI_Win_flush_local(ptr.rank, BCL::win);
    return result;
  }
}
