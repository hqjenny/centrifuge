
#pragma once

#include <stdexcept>
#include "backend.hpp"

namespace BCL {

template <typename T>
class future {
public:
  T value_;
  MPI_Request request_ = MPI_REQUEST_NULL;

  future() {}

  future(const T& value, const MPI_Request& request)
    : value_(value), request_(request) {}

  future(T&& value, const MPI_Request& request)
    : value_(std::move(value)), request_(request) {}

  future(future&& future_)
    : value_(std::move(future_.value_)), request_(std::move(future_.request_)) {}

  future& operator=(future&& future_) {
    value_ = std::move(future_.value_);
    request_ = std::move(future_.request_);
    return *this;
  }

  void wait_() {
    if (request_ == MPI_REQUEST_NULL) {
      throw std::runtime_error("future: waiting on an expired future");
    }
    MPI_Wait(&request_, MPI_STATUS_IGNORE);
  }

  // Maybe const? What does it mean for a future to be const?
  T get() {
    wait_();
    return T(std::move(value_));
  }
};

}
