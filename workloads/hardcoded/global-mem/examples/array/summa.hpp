
#pragma once

#include <BCL.hpp>
#include <containers/DMatrix.hpp>

#include "gemx_tl.hpp"
#include "cblas_wrapper.hpp"

namespace BCL {
namespace experimental {

enum BlasTrans {
  notrans,
  trans
};

double compute = 0.0;
double issue = 0.0;
double sync = 0.0;
double load_imbalance = 0.0;

template <typename T>
void gemm_notrans_impl_(BCL::DMatrix<T>& a, BCL::DMatrix<T>& b, BCL::DMatrix<T>& c);

enum ENGINE {cpu, accel};
template <typename T>
void gemm(BlasTrans transa, BlasTrans transb, BCL::DMatrix<T>& a, BCL::DMatrix<T>& b, BCL::DMatrix<T>& c, ENGINE engine) {
  if (transa == BlasTrans::notrans && transb == BlasTrans::notrans) {
    gemm_notrans_impl_(a, b, c, engine);
  } else {
    throw std::runtime_error("SUMMA: not implemented yet.\n");
  }
}

template <typename T>
void gemm_notrans_impl_(BCL::DMatrix<T>& a, BCL::DMatrix<T>& b, BCL::DMatrix<T>& c, ENGINE engine) {
  if (!(a.m() == c.m() && a.n() == b.m() && b.n() == c.n())) {
    throw std::runtime_error("SUMMA: ruh roh, you gave me matrices with mismatched dimensions.");
  }

  for (size_t i = 0; i < c.grid_dim_m(); i++) {
    for (size_t j = 0; j < c.grid_dim_n(); j++) {
      if (c.submatrix(i, j).is_local()) {

        auto begin = std::chrono::high_resolution_clock::now();
        size_t k_offset = j;
        auto buf_a = a.arget_submatrix(i, k_offset % a.grid_dim_n());
        auto buf_b = b.arget_submatrix(k_offset % a.grid_dim_n(), j);
        BCL::GlobalPtr<T> my_c = c.submatrix(i, j);
        auto end = std::chrono::high_resolution_clock::now();
        issue += std::chrono::duration<double>(end - begin).count();
        for (size_t k_ = 0; k_ < a.grid_dim_n(); k_++) {
          size_t k = (k_ + k_offset) % a.grid_dim_n();

          auto begin = std::chrono::high_resolution_clock::now();
          BCL::GlobalPtr<T> my_a = buf_a.get();
          BCL::GlobalPtr<T> my_b = buf_b.get();
          auto end = std::chrono::high_resolution_clock::now();
          sync += std::chrono::duration<double>(end - begin).count();

          if (k_+1 < a.grid_dim_n()) {
            begin = std::chrono::high_resolution_clock::now();
            buf_a = a.arget_submatrix(i, (k+1) % a.grid_dim_n());
            buf_b = b.arget_submatrix((k+1) % a.grid_dim_n(), j);
            end = std::chrono::high_resolution_clock::now();
            issue += std::chrono::duration<double>(end - begin).count();
          }

          size_t M, N, K;
          size_t lda = a.tile_size_n();
          size_t ldb = b.tile_size_n();
          size_t ldc = c.tile_size_n();
          std::tie(M, N) = c.submatrix_size(i, j);
          std::tie(M, K) = a.submatrix_size(i, k);

          begin = std::chrono::high_resolution_clock::now();
          if (engine == cpu) {
            cblas_gemm_wrapper_(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                      M, N, K,
                      1, my_a.local(), lda,
                      my_b.local(), ldb, 1,
                      my_c.local(), ldc);
          } else {
            gemx_accel(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                      M, N, K,
                      1, my_a.local(), lda,
                      my_b.local(), ldb, 1,
                      my_c.local(), ldc);

          }
          end = std::chrono::high_resolution_clock::now();
          compute += std::chrono::duration<double>(end - begin).count();
        }
      }
    }
  }
}

}
}
