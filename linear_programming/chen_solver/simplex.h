/*
 * Created by Zhen Chen on 2025/3/8.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef SIMPLEX_H
#define SIMPLEX_H
#include <algorithm>
#include <optional>
#include <vector>

constexpr double M = 10000;
constexpr double threshold = 1e-6;
constexpr bool bool_record_tableau = true;

// enum class Comparison { LessOrEqual, Equal, GreaterOrEqual };
enum class AntiCycle { None, Bland, Lexicography };

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

class Simplex {
  std::vector<double> obj_coe;
  int obj_sense{}; // 0:min, 1: max
  std::vector<std::vector<double>> con_lhs;
  std::vector<double> con_rhs;
  std::vector<int> constraint_sense; // 0:<=, 1: >=, 2: =
  std::vector<int> var_sign;         // 0: >=, 1: <=, 2: unsigned

  AntiCycle anti_cycle{AntiCycle::None};
  bool obj_sense_changed{};
  int solution_status = {3}; // 0 optimal, 1 unbounded, 2 infeasible, 3 unsolved

  std::vector<std::vector<double>> tableau;
  int constraint_num{}; // number of constraints
  int var_total_num{};
  int var_original_num{};
  int var_unsigned_num{};
  std::vector<int> front_unsigned_num; // unsigned var number before a var
  int var_slack_num{};
  int var_artificial_num{};
  std::vector<int> con_slack_coe; // the coefficient of the slack variable in the constraint
  std::vector<int>
      con_artificial_coe;      // the coefficient of the artificial variable in the constraint
  std::vector<int> basic_vars; // basic var index for each constraint row

  std::vector<std::vector<std::vector<double>>> recorded_tableau;
  std::vector<std::vector<int>> recorded_pivot_index;

  // 找到主列（进入变量）
  [[nodiscard]] int findPivotColumn() const;

  // 找到主行（离开变量）
  [[nodiscard]] int findPivotRow(int pivot_column) const;

  // compute reduced cost
  [[nodiscard]] double computeReduceCost(const std::vector<double> &cB, int Ai_index) const;

  int artificialIndexInBasicVars();

  // 行变换
  void pivot(int pivot_row, int pivot_column);

public:
  Simplex(const int obj_sense, const std::vector<double> &obj_coe,
          const std::vector<std::vector<double>> &con_lhs, const std::vector<double> &con_rhs,
          const std::vector<int> &constraint_sense, const std::vector<int> &var_sign)
      : obj_coe(obj_coe), obj_sense(obj_sense), con_lhs(con_lhs), con_rhs(con_rhs),
        constraint_sense(constraint_sense), var_sign(var_sign) {
    constraint_num = static_cast<int>(con_lhs.size());
    var_total_num = static_cast<int>(var_sign.size());
    var_original_num = var_total_num;
    recorded_tableau.reserve(10);
    recorded_pivot_index.reserve(10);
  };

  // single argument constructor must be explicit
  explicit Simplex(const std::vector<std::vector<double>> &initialTableau) {
    tableau = initialTableau;
    constraint_num = static_cast<int>(tableau.size()) - 1;
    var_total_num = static_cast<int>(tableau[0].size()) - 1;
    initializeBasicVariables(); // 初始化基变量
  }

  [[nodiscard]] int getStatus() const { return solution_status; }

  void standardize();
  void print() const;
  void printConLHS() const;

  void checkInput() const;

  [[nodiscard]] double testWeb() const;

  void printTableau() const;
  void printAllTableau() const;

  void setAntiCycle(AntiCycle rule);

  void inputObjCoef() const;

  void solve();
  // solve the simpler tableau in 2-stage computation
  void solve2(std::vector<std::vector<double>> &tableau);

  [[nodiscard]] int isBasicVariable(int column_index) const;

  void displaySolution() const;
  void displayBasicSolution() const;

  void initializeBasicVariables();

  void initializeObjective();

  [[nodiscard]] std::optional<double> getOptValue() const;
  [[nodiscard]] std::vector<double> getOptSolution() const;
  [[nodiscard]] std::vector<int> getBasicVarsIndices() const { return basic_vars; };
  [[nodiscard]] std::vector<std::vector<double>> getTableau() const { return tableau; };

  std::vector<std::vector<std::vector<double>>> get_recorded_tableau() const {
    return recorded_tableau;
  };
  std::vector<std::vector<int>> get_recorded_pivot_index() const { return recorded_pivot_index; };
};

#endif // SIMPLEX_H
