
#pragma once

#include <BCL.hpp>
#include <vector>
#include <stdexcept>
#include <iostream>

namespace BCL
{

template <typename T>
class GlobalRef {
public:

  BCL::GlobalPtr<T> ptr_;

  GlobalRef(const BCL::GlobalPtr<T>& ptr) : ptr_(ptr) {
    if (ptr_ == nullptr) {
      throw std::runtime_error("GlobalRef: you made a null reference!");
    }
  }

  operator T() const {
    return BCL::rget(ptr_);
  }

  GlobalRef &operator=(const T& value) {
    BCL::rput(value, ptr_);
    return *this;
  }

  BCL::GlobalPtr<T> operator&() {
    return ptr_;
  }
};

template <typename T>
class DMatrix {
public:

  std::vector<BCL::GlobalPtr<T>> ptrs_;

  // Size of *matrix* (in elements)
  size_t m_, n_;
  // Size of *processor* grid
  size_t pm_, pn_;
  // Size (in elements) of a *tile*
  size_t tile_size_m_, tile_size_n_;

  // Size of *tile* grid (in tiles)
  size_t grid_dim_m_, grid_dim_n_;

  DMatrix(size_t m, size_t n, size_t pm, size_t pn, size_t tile_size_m,
          size_t tile_size_n) : m_(m), n_(n), pm_(pm), pn_(pn),
                                tile_size_m_(tile_size_m),
                                tile_size_n_(tile_size_n) {
    if (pm*pn > BCL::nprocs()) {
      throw std::runtime_error("DMatrix: tried to create a DMatrix with a too large p-grid.");
    }

    grid_dim_m_ = (m + tile_size_m - 1) / tile_size_m;
    grid_dim_n_ = (n + tile_size_n - 1) / tile_size_n;

    for (size_t i = 0; i < grid_dim_m(); i++) {
      for (size_t j = 0; j < grid_dim_n(); j++) {
        size_t lpi = i % pm;
        size_t lpj = j % pn;
        size_t proc = lpj + lpi*pn;
        BCL::GlobalPtr<T> ptr;
        if (BCL::rank() == proc) {
          ptr = BCL::alloc<T>(tile_size());
        }
        ptr = BCL::broadcast(ptr, proc);
        if (ptr == nullptr) {
          throw std::runtime_error("DMatrix: ran out of memory!");
        }
        ptrs_.push_back(ptr);
      }
    }
    BCL::barrier();
  }

  BCL::GlobalPtr<T> submatrix(size_t i, size_t j) {
    return ptrs_[j + i*grid_dim_n()];
  }

  BCL::future<BCL::GlobalPtr<T>> arget_submatrix(size_t i, size_t j) {
    return BCL::arget(submatrix(i, j), tile_size());
  }

  std::vector<T> get_submatrix(size_t i, size_t j) {
    std::vector<T> vals(tile_size());
    BCL::rget(submatrix(i, j), vals.data(), tile_size());
    return vals;
  }

  GlobalRef<T> operator()(size_t i, size_t j) {
    size_t pi = i / tile_size_m();
    size_t pj = j / tile_size_n();
    size_t p = pj + pi*grid_dim_n();

    size_t local_i = i - pi*tile_size_m();
    size_t local_j = j - pj*tile_size_n();
    size_t local_idx = local_j + local_i*tile_size_n();
    return GlobalRef<T>(ptrs_[p] + local_idx);
  }

  const GlobalRef<T> operator()(size_t i, size_t j) const {
    size_t pi = i / tile_size_m();
    size_t pj = j / tile_size_n();
    size_t p = pj + pi*grid_dim_n();

    size_t local_i = i - pi*tile_size_m();
    size_t local_j = j - pj*tile_size_n();
    size_t local_idx = local_j + local_i*tile_size_n();
    return GlobalRef<T>(ptrs_[p] + local_idx);
  }

  template <typename U>
  DMatrix& operator=(const U& value) {
    for (size_t i = 0; i < ptrs_.size(); i++) {
      if (ptrs_[i].is_local()) {
        for (size_t j = 0; j < tile_size(); j++) {
          ptrs_[i].local()[j] = value;
        }
      }
    }
    return *this;
  }

  size_t m() const noexcept {
    return m_;
  }

  size_t n() const noexcept {
    return n_;
  }

  size_t grid_dim_m() const noexcept {
    return grid_dim_m_;
  }

  size_t grid_dim_n() const noexcept {
    return grid_dim_n_;
  }

  size_t tile_size_m() const noexcept {
    return tile_size_m_;
  }

  size_t tile_size_n() const noexcept {
    return tile_size_n_;
  }

  size_t tile_size() const noexcept {
    return tile_size_m()*tile_size_n();
  }

  size_t pm() const noexcept {
    return pm_;
  }

  size_t pn() const noexcept {
    return pn_;
  }

  size_t tiles_per_proc_m() const noexcept {
    return (grid_dim_m() + pm() - 1) / pm();
  }

  size_t tiles_per_proc_n() const noexcept {
    return (grid_dim_n() + pn() - 1) / pn();
  }

  size_t tiles_per_proc() const noexcept {
    return tiles_per_proc_m() * tiles_per_proc_n();
  }

  std::tuple<size_t, size_t> submatrix_size(size_t i, size_t j) const noexcept {
    size_t m_size = std::min(tile_size_m(), m() - i*tile_size_m());
    size_t n_size = std::min(tile_size_n(), n() - j*tile_size_n());
    return std::make_pair(m_size, n_size);
  }

  // TODO: Should really do this by iterating
  // through submatrices...
  // What's a nice syntax for that?
  std::vector<T> get_matrix() const {
    std::vector<T> my_matrix(m()*n());
    for (size_t i = 0; i < m(); i++) {
      for (size_t j = 0; j < n(); j++) {
        my_matrix[i*n() + j] = (*this)(i, j);
      }
    }
    return my_matrix;
  }

  void print() const {
    for (size_t i = 0; i < m(); i++) {
      for (size_t j = 0; j < n(); j++) {
        std::cout << (*this)(i, j) << " ";
      }
      std::cout << std::endl;
    }
  }
};

template<typename T, typename U>
void fill_range(DMatrix<T>& mat, U bound) {
  for (size_t gi = 0; gi < mat.grid_dim_m(); gi++) {
    for (size_t gj = 0; gj < mat.grid_dim_n(); gj++) {
      if (mat.submatrix(gi, gj).is_local()) {
        for (size_t i = 0; i < mat.tile_size_m(); i++) {
          for (size_t j = 0; j < mat.tile_size_n(); j++) {
            size_t i_ = gi * mat.tile_size_m();
            size_t j_ = gj * mat.tile_size_n();
            mat.submatrix(gi, gj).local()[i*mat.tile_size_n() + j] = ((i_*mat.n() + j_) % bound);
          }
        }
      }
    }
  }
}

}
