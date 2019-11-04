#pragma once

namespace BCL {
  // Backend must fulfill these methods.
  template <typename T>
  void read(const GlobalPtr <T> &src, T *dst, const size_t size);

  template <typename T>
  void atomic_read(const GlobalPtr <T> &src, T *dst, const size_t size);

  template <typename T>
  void write(const T *src, const GlobalPtr <T> &dst, const size_t size);

  template <typename T>
  void rput(const T &src, const GlobalPtr <T> &dst) {
    BCL::write(&src, dst, 1);
  }

  template <typename T>
  void rput(const T *src, const GlobalPtr <T> &dst, const size_t size) {
    BCL::write(src, dst, size);
  }

  template <typename T>
  void rget(const GlobalPtr <T> &src, T *dst, const size_t size) {
    BCL::read(src, dst, size);
  }

  template <typename T>
  void rget_atomic(const GlobalPtr <T> &src, T *dst, const size_t size) {
    BCL::atomic_read(src, dst, size);
  }

  template <typename T>
  T rget_atomic(const GlobalPtr <T> &src) {
    T rv;
    BCL::atomic_read(src, &rv, 1);
    return rv;
  }

  template <typename T>
  T rget(const GlobalPtr <T> &src) {
    T rv;
    BCL::read(src, &rv, 1);
    return rv;
  }
}
