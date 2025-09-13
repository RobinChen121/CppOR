/*
 * Created by Zhen Chen on 2025/3/8.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef SIMPLEX_H
#define SIMPLEX_H
#include <vector>

enum class Comparison { LessOrEqual, Equal, GreaterOrEqual };

enum class VarSign {
  NonNegative, // ≥ 0
  NonPositive  // ≤ 0
};

class Simplex {
private:
  std::vector<double> obj_coe;
  int obj_sense{}; // 0:min, 1: max
  std::vector<std::vector<double>> con_lhs;
  std::vector<double> con_rhs;
  std::vector<int> con_sense; // 0:>=, 1: <=, 2: =
  std::vector<int> var_sign;  // 0: >=, 1: <=, 2: unsigned

  std::vector<std::vector<double>> tableau; // 单纯形表
  int con_num{};                            // number of constraints
  int var_num{};
  std::vector<bool> con_has_slack; // whether the constraint has a slack variable
  std::vector<bool> con_has_artificial;
  std::vector<int> basicVars;

  // 找到主列（进入变量）
  [[nodiscard]] int findPivotColumn() const;

  // 找到主行（离开变量）
  [[nodiscard]] int findPivotRow(int pivotCol) const;

  // 行变换
  void pivot(int pivotRow, int pivotCol);

public:
  Simplex(const std::vector<double> &obj_coe, const std::vector<std::vector<double>> &con_lhs,
          const std::vector<double> &con_rhs, const std::vector<int> &con_sense,
          const std::vector<int> &var_sign)
      : obj_coe(obj_coe), con_lhs(con_lhs), con_rhs(con_rhs), con_sense(con_sense),
        var_sign(var_sign) {};

  explicit Simplex(const std::vector<std::vector<double>> &initialTableau);

  void standardize();

  void inputObjCoef() const;

  void solve();

  [[nodiscard]] int isBasicVariable(int col) const;

  void displaySolution() const;

  void initializeBasicVariables();

  void initializeObjective();

  void displayTableau() const;

  std::vector<std::vector<double>> generateTableau();
};

#endif // SIMPLEX_H
