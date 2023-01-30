#include <algorithm>
#include <cassert>
#include <iostream>
#include <thread>
#if defined(_OPENMP)
#include <omp.h>
#endif
#include "ProdMatMat.hpp"

namespace {
void prodSubBlocks(int iRowBlkA, int iColBlkB, int iColBlkA, int szBlock,
                   const Matrix& A, const Matrix& B, Matrix& C) {

      #pragma omp parallel for num_threads(8)
     //#pragma omp parallel for
     for (int j = iColBlkB; j < std::min(B.nbCols, iColBlkB + szBlock); j++){
      for (int k = iColBlkA; k < std::min(A.nbCols, iColBlkA + szBlock); k++){
       for (int i = iRowBlkA; i < std::min(A.nbRows, iRowBlkA + szBlock); ++i)
        {
        C(i, j) += A(i, k) * B(k, j);
        //std::cout<<"C("<<i<<","<<j<<") = "<<"A("<<i<<","<<k<<")*B("<<k<<","<<j<<")\n";
        }
      }
    }
        
}
//const int szBlock = 32;
}  // namespace

Matrix operator*(const Matrix& A, const Matrix& B) {
  Matrix C(A.nbRows, B.nbCols, 0.0);

  int szBlock = 256;
  for (int iRowBlkA = 0; iRowBlkA < A.nbRows; iRowBlkA += szBlock)
    for (int iColBlkB = 0; iColBlkB < B.nbCols; iColBlkB += szBlock)
      for (int iColBlkA = 0; iColBlkA < A.nbCols; iColBlkA += szBlock)
        prodSubBlocks(iRowBlkA, iColBlkB, iColBlkA, szBlock, A, B, C);

  return C;
  
}



