#pragma once

#include <BCL.hpp>

// TODO: Exceptions, new() allocator which
// calls constructor, rdealloc()

namespace BCL {
  template <typename T>
  GlobalPtr <T> alloc(const size_t size) {
    return local_malloc <T> (size);
  }

  template <typename T>
  void dealloc(GlobalPtr <T> ptr) {
    local_free <T> (ptr);
  }

  template <typename T>
  GlobalPtr <T> ralloc(const size_t size, const uint64_t rank);

  template <typename T>
  void rdealloc(GlobalPtr <T> &ptr);
}
