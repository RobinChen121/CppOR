/*
 * Created by Zhen Chen on 2026/3/20.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "simplex.h"

#include <iostream>

CSC Simplex::denseToCSC(const std::vector<std::vector<double>> &A) {
  const int m = static_cast<int>(A.size());    // numer of row
  const int n = static_cast<int>(A[0].size()); // number of column

  const int nnz = m * n;
  if (m * n > 1e6) // to-do
    return {};
  CSC csc;
  csc.non_zero_values.reserve(nnz);
  csc.col_start_end.push_back(0);

  for (int j = 0; j < n; j++) { // scan from column
    for (int i = 0; i < m; i++) {
      if (std::abs(A[i][j]) > eps) {
        csc.non_zero_values.push_back(A[i][j]);
        csc.row_indices.push_back(i);
      }
    }
    csc.col_start_end.push_back(static_cast<int>(csc.non_zero_values.size()));
  }

  return csc;
}

void Simplex::printCSC(const CSC &csc) {
  std::cout << "values: ";
  for (const auto v : csc.non_zero_values)
    std::cout << v << " ";
  std::cout << std::endl;

  std::cout << "row_idx: ";
  for (const auto r : csc.row_indices)
    std::cout << r << " ";
  std::cout << std::endl;

  std::cout << "col_ptr: ";
  for (const auto c : csc.col_start_end)
    std::cout << c << " ";
  std::cout << std::endl;
}

int main() {
  const std::vector<std::vector<double>> A = {{1, 0, 2}, {0, 3, 0}, {4, 0, 5}};

  const CSC csc = Simplex::denseToCSC(A);
  Simplex::printCSC(csc);

  return 0;
}