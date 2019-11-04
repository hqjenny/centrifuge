#pragma once

#include <mpp/shmem.h>

namespace BCL {
  // Ops define an MPI OP and an MPI Datatype
  template <typename T>
  struct abstract_op {
  };

  // Some special atomic ops also offer
  // a shmem atomic version
  template <typename T>
  struct atomic_op : public virtual abstract_op <T> {
    virtual T shmem_atomic_op(const GlobalPtr <T> ptr, const T &val) const = 0;
  };

  // Define datatypes
  struct abstract_int : public virtual abstract_op <int> {
  };

  struct abstract_uint64_t : public virtual abstract_op <uint64_t> {
  };

  struct abstract_float : public virtual abstract_op <float> {
  };

  struct abstract_double : public virtual abstract_op <double> {
  };

  // Define the plus operation
  template <typename T>
  struct abstract_plus : public virtual abstract_op <T> {
  };

  template <typename T> struct plus;

  template <>
  struct plus <uint64_t> : public abstract_plus <uint64_t>, public abstract_uint64_t {
  };

  template <>
  struct plus <int> : public abstract_plus <int>, public abstract_int, public atomic_op <int> {
    int shmem_atomic_op(const GlobalPtr <int> ptr, const int &val) const {
      return shmem_int_fadd(ptr.rptr(), val, ptr.rank);
    }
  };

  template <>
  struct plus <float> : public abstract_plus <float>, public abstract_float {};

  template <>
  struct plus <double> : public abstract_plus <double>, public abstract_double {};

  template <typename T>
  struct abstract_land : public virtual abstract_op <T> {
  };

  template <typename T>
  struct abstract_max : public virtual abstract_op <T> {
  };

  template <typename T> struct land;
  template <typename T> struct max;

  template <>
  struct land <int> : public abstract_land <int>, public abstract_int {};

  template <>
  struct max <int> : public abstract_max <int>, public abstract_int {};
}
