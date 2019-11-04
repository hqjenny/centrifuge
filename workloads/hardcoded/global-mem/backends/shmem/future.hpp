
#pragma once

#include <stdexcept>
#include "backend.hpp"

namespace BCL {

template <typename T>
class future {
public:
  T value_;

  future() {}

  future(const T& value)
    : value_(value) {}

  future(T&& value)
    : value_(std::move(value)) {}

  future(future&& future_)
    : value_(std::move(future_.value_)) {}

  future& operator=(future&& future_) {
    value_ = std::move(future_.value_);
    return *this;
  }

  void wait_() {
    shmem_quiet();
  }

  // Maybe const? What does it mean for a future to be const?
  T get() {
    wait_();
    return T(std::move(value_));
  }
};

}
