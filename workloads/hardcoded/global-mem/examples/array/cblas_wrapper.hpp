
#pragma once

//#include <cblas.h>
#include <vector>
namespace BCL {
  namespace experimental {

 /*   void cblas_gemm_wrapper_(const CBLAS_LAYOUT layout,
        const CBLAS_TRANSPOSE transa, const CBLAS_TRANSPOSE transb,
        const int m, const int n, const int k,
        const float alpha, const float* a, const int lda,
        const float* b, const int ldb, const float beta,
        float* c, const int ldc) {
      cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
          m, n, k,
          alpha, a, lda,
          b, ldb, beta,
          c, ldc);
    }

    void cblas_gemm_wrapper_(const CBLAS_LAYOUT layout,
        const CBLAS_TRANSPOSE transa, const CBLAS_TRANSPOSE transb,
        const int m, const int n, const int k,
        const double alpha, const double* a, const int lda,
        const double* b, const int ldb, const double beta,
        double* c, const int ldc) {
      cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
          m, n, k,
          alpha, a, lda,
          b, ldb, beta,
          c, ldc);
    }
*/
typedef enum {CblasRowMajor=101, CblasColMajor=102} CBLAS_LAYOUT;
typedef enum {CblasNoTrans=111, CblasTrans=112, CblasConjTrans=113} CBLAS_TRANSPOSE;
typedef enum {CblasUpper=121, CblasLower=122} CBLAS_UPLO;
typedef enum {CblasNonUnit=131, CblasUnit=132} CBLAS_DIAG;
typedef enum {CblasLeft=141, CblasRight=142} CBLAS_SIDE;


#define BLK_SIZE 128
#define min(a,b) (((a)<(b))?(a):(b))
    // c = alpha * ab + beta * c
    void cblas_gemm_wrapper_(const CBLAS_LAYOUT layout,
        const CBLAS_TRANSPOSE transa, const CBLAS_TRANSPOSE transb,
        const int m, const int n, const int k,
        const int alpha, const int* a, const int lda,
        const int* b, const int ldb, const int beta,
        int* c, const int ldc) {
      //cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
      //            m, n, k,
      //            alpha, a, lda,
      //            b, ldb, beta,
      //            c, ldc);
      // Implement integer gemm 

      // Assuming RowMajor, NoTrans 
      int i_, j_, k_, ii, jj, kk, Aik, bs = BLK_SIZE;

      //FROM A m * n B n * p C m * p
      //TO A m * k B k * n C m * n
      for(ii = 0; ii < m; ii += bs)
        for(kk = 0; kk < k; kk += bs)
          for(jj = 0; jj < n; jj += bs)
            for(i_ = ii; i_ < min(m, ii+bs); i_++)
              for(k_ = kk; k_ < min(k, kk+bs); k_++)
              {
                //Aik = a[k*i_+k_];
                Aik = a[lda*i_+k_];
                for(j_ = jj; j_ < min(n, jj+bs); j_+=8)
                {
                  c[ldc*i_+j_] += alpha * Aik * b[ldb*k_+j_];
                  c[ldc*i_+j_+1] += alpha * Aik * b[ldb*k_+j_+1];		
                  c[ldc*i_+j_+2] += alpha * Aik * b[ldb*k_+j_+2];	
                  c[ldc*i_+j_+3] += alpha * Aik * b[ldb*k_+j_+3];	
                  c[ldc*i_+j_+4] += alpha * Aik * b[ldb*k_+j_+4];		
                  c[ldc*i_+j_+5] += alpha * Aik * b[ldb*k_+j_+5];	
                  c[ldc*i_+j_+6] += alpha * Aik * b[ldb*k_+j_+6];	
                  c[ldc*i_+j_+7] += alpha * Aik * b[ldb*k_+j_+7];	
                }
              }	

    }

#define BLK_SIZE 128
#define min(a,b) (((a)<(b))?(a):(b))
    // c = alpha * ab + beta * c
    void cblas_gemm_wrapper_(const CBLAS_LAYOUT layout,
        const CBLAS_TRANSPOSE transa, const CBLAS_TRANSPOSE transb,
        const int m, const int n, const int k,
        const short alpha, const short* a, const int lda,
        const short* b, const int ldb, const short beta,
        short* c, const int ldc) {
      //cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
      //            m, n, k,
      //            alpha, a, lda,
      //            b, ldb, beta,
      //            c, ldc);
      // Implement integer gemm 

      // Assuming RowMajor, NoTrans 
      int i_, j_, k_, ii, jj, kk, Aik, bs = BLK_SIZE;

      //FROM A m * n B n * p C m * p
      //TO A m * k B k * n C m * n
      for(ii = 0; ii < m; ii += bs)
        for(kk = 0; kk < k; kk += bs)
          for(jj = 0; jj < n; jj += bs)
            for(i_ = ii; i_ < min(m, ii+bs); i_++)
              for(k_ = kk; k_ < min(k, kk+bs); k_++)
              {
                //Aik = a[k*i_+k_];
                Aik = a[lda*i_+k_];
                for(j_ = jj; j_ < min(n, jj+bs); j_+=8)
                {
                  c[ldc*i_+j_] += alpha * Aik * b[ldb*k_+j_];
                  c[ldc*i_+j_+1] += alpha * Aik * b[ldb*k_+j_+1];		
                  c[ldc*i_+j_+2] += alpha * Aik * b[ldb*k_+j_+2];	
                  c[ldc*i_+j_+3] += alpha * Aik * b[ldb*k_+j_+3];	
                  c[ldc*i_+j_+4] += alpha * Aik * b[ldb*k_+j_+4];		
                  c[ldc*i_+j_+5] += alpha * Aik * b[ldb*k_+j_+5];	
                  c[ldc*i_+j_+6] += alpha * Aik * b[ldb*k_+j_+6];	
                  c[ldc*i_+j_+7] += alpha * Aik * b[ldb*k_+j_+7];	
                  //if(i_ == 0 && j_ == 0) printf("%d ", c[0]);
                }
              }	

    }
    template <typename T>
      std::vector<T> cblas_test(const std::vector<T>& a, const std::vector<T>& b,
          size_t M, size_t N, size_t K) {
        std::vector<T> c(M*N);
        cblas_gemm_wrapper_(CblasRowMajor, CblasNoTrans, CblasNoTrans,
            M, N, K,
            1.0, a.data(), K,
            b.data(), N, 1.0,
            c.data(), N);
        return c;
      }

  }
}
