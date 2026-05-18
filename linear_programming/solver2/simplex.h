/*
 * Created by Zhen Chen on 2026/3/20.
 * Email: chen.zhen5526@gmail.com
 * Description: This simplex algorithm aims to process sparse matrix.
 *
 *
 */

#ifndef WORKFORCE_SIMPLEX_H
#define WORKFORCE_SIMPLEX_H

#include <algorithm>
#include <chrono>
#include <iostream>
#include <optional>
#include <set>
#include <unordered_set>
#include <vector>

constexpr double eps = 1e-6;
constexpr bool bool_record_tableau = true;
constexpr int anti_cycle_rule = 0; // 0: None, 1: Bland, 2: Lexicography
// inline 用来声明全局的动态数组，若这个数组最终没有使用，只浪费 24 或 32 的对象内存开销
inline bool detectCycling(std::vector<int> current_basis, std::set<std::vector<int>> basis_history);

// 删除一个矩阵指定的多个列
template <typename T>
void eraseColumns(std::vector<std::vector<T>> &matrix, std::vector<int> columns) {
  // 删除重复列并按降序排序
  // rbegin() 和 rend() 分别返回 反向迭代器（reverse iterator）的起点和终点
  std::sort(columns.rbegin(), columns.rend());
  columns.erase(std::ranges::unique(columns).begin(), columns.end());

  for (auto &row : matrix) {
    for (size_t col : columns) {
      if (col < row.size()) {
        row.erase(row.begin() + col);
      }
    }
  }
}

// compressed sparse column
struct CSC {
  std::vector<double> non_zero_values;
  std::vector<int> row_indices; // row indices for the non-zero values
  // the number of the following values always 1 larger than the number of columns
  std::vector<int> col_start_end; // column start and end indices in the non-zero values

  CSC() = default; // 空对象方便临时使用
  // nnz = 预计非零元素个数
  // cols = 矩阵列数
  // 工业级时最好估计非零个数，从而提前分配内存
  // array 也可以直接用数值初始化，此时元素都为0
  CSC(const int nnz, const int cols)
      : non_zero_values(nnz), row_indices(nnz), col_start_end(cols + 1) {}
};

void printCSC(const CSC &csc);
CSC denseToCSC(const std::vector<std::vector<double>> &A);
double getValue(const CSC &csc, int row,
                int col); // get the value for element in A from CSC

class Simplex {
  // --------------------
  // original inputs
  std::vector<double> obj_coe;
  int obj_sense{}; // 0:min, 1: max
  std::vector<std::vector<double>> con_lhs;
  std::vector<double> con_rhs;
  std::vector<int> constraint_sense; // 0:<=, 1: >=, 2: =
  std::vector<int> var_sign;         // 0: >=, 1: <=, 2: unsigned

  // --------------------
  // middle attributes

  int solution_status = {3}; // 0 optimal, 1 unbounded, 2 infeasible, 3 unsolved, 4 cycling
  int var_num_start{};       // var number at the beginning
  int var_num_total{};       // var number later
  int var_num_unsigned{};
  // for outputting the final solution
  // unsigned var number before a var
  std::vector<int> front_unsigned_num;
  int con_num{};
  int var_num_slack{};
  int var_num_artificial{};
  // if one element is 0, it means no slack or artificial variable in this constraint
  // the coefficient of the corresponding slack variable for each constraint
  std::vector<int> con_coe_slack;
  std::vector<int> con_coe_artificial; // the coefficient of the corresponding artificial variable
  // for each constraint
  std::vector<int> basic_var_index; // basic var index for each constraint row
  bool obj_sense_changed{false};
  double run_time{};

  std::vector<std::vector<double>> tableau;
  std::vector<std::vector<std::vector<double>>> recorded_tableau;
  int anti_cycle_rule{};                        // 0: None, 1: Bland, 2: Lexicography
  std::vector<std::vector<int>> recorded_pivot; // out and in
  std::set<std::vector<int>> recorded_basic_var_index;

public:
  Simplex(const int obj_sense, const std::vector<double> &obj_coe,
          const std::vector<std::vector<double>> &con_lhs, const std::vector<double> &con_rhs,
          const std::vector<int> &constraint_sense, const std::vector<int> &var_sign,
          const int anti_cycle_rule)
      : obj_coe(obj_coe), obj_sense(obj_sense), con_lhs(con_lhs), con_rhs(con_rhs),
        constraint_sense(constraint_sense), var_sign(var_sign), anti_cycle_rule(anti_cycle_rule) {
    con_num = static_cast<int>(con_lhs.size());
    var_num_start = static_cast<int>(var_sign.size());
    var_num_total = var_num_start;
    if constexpr (bool_record_tableau) {
      recorded_tableau.reserve(50);
      if (anti_cycle_rule == 0) {
        recorded_pivot.reserve(50);
      }
    }
  };

  void standardize(); // many solvers call this step as presolve()
  void checkInput() const;
  void print() const;
  void printAllTableau() const;
  void initializeBasicVars();
  // 判断某列是否为基变量，并返回对应的行（如果不是基变量，返回 -1）
  [[nodiscard]] int isBasicVar(int column_index) const;

  void solve();
  void firstStage();
  [[nodiscard]] std::vector<double> computeReduceCostAll(const std::vector<double> &obj_coe_) const;
  [[nodiscard]] double computeReduceCostSingle(const std::vector<double> &cB, int Ai_index) const;
  int artificialIndexInBasicVars();
  void singleStage();
  [[nodiscard]] std::optional<double> getOptValue() const;
  [[nodiscard]] int findPivotRow(int pivot_column) const;
  [[nodiscard]] std::vector<double> getOptSolution() const;
  void displaySolution() const;
  void pivot(int pivot_row, int pivot_column);
  void printTableau() const;
  [[nodiscard]] int findPivotColumn() const;
  void setAntiCycle(int rule);
  [[nodiscard]] int getStatus() const { return solution_status; }
  [[nodiscard]] std::vector<int> getBasicVarsIndices() const { return basic_var_index; }
  [[nodiscard]] std::vector<std::vector<std::vector<double>>> get_recorded_tableau() const {
    return recorded_tableau;
  }
  [[nodiscard]] std::vector<std::vector<int>> get_recorded_pivot() const { return recorded_pivot; }
  [[nodiscard]] double getTime() const { return run_time; }
};

#endif // WORKFORCE_SIMPLEX_H
