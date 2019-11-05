#pragma once

#include <BCL.hpp>

namespace BCL {
  template <typename ...Args>
  void print(std::string format, Args... args) {
    fflush(stdout);
    BCL::barrier();
    if (BCL::rank() == 0) {
      printf(format.c_str(), args...);
    }
    fflush(stdout);
    BCL::barrier();
  }
}
