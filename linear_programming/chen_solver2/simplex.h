/*
 * Created by Zhen Chen on 2026/3/20.
 * Email: chen.zhen5526@gmail.com
 * Description: This simplex algorithm aims to process sparse matrix.
 *
 *
 */

#ifndef WORKFORCE_SIMPLEX_H
#define WORKFORCE_SIMPLEX_H

#include <vector>

constexpr double eps = 1e-12;

// compressed sparse column
struct CSC {
  std::vector<double> non_zero_values;
  std::vector<int> row_indices;   // row indices for the non-zero values
  std::vector<int> col_start_end; // column start and end indices in the non-zero values

  CSC() = default; // 空对象方便临时使用
  // nnz = 预计非零元素个数
  // cols = 矩阵列数
  // 工业级时最好估计非零个数，从而提前分配内存
  // array 也可以直接用数值初始化，此时元素都为0
  CSC(const int nnz, const int cols)
      : non_zero_values(nnz), row_indices(nnz), col_start_end(cols + 1) {}
};

class Simplex {
  std::vector<double> obj_coe;
  int obj_sense{}; // 0:min, 1: max
  std::vector<std::vector<double>> con_lhs;
  std::vector<double> con_rhs;
  std::vector<int> constraint_sense; // 0:<=, 1: >=, 2: =
  std::vector<int> var_sign;         // 0: >=, 1: <=, 2: unsigned
  int solution_status = {3};         // 0 optimal, 1 unbounded, 2 infeasible, 3 unsolved

public:
  Simplex();

  static CSC denseToCSC(const std::vector<std::vector<double>> &A);
  static void printCSC(const CSC &csc);
};

#endif // WORKFORCE_SIMPLEX_H
