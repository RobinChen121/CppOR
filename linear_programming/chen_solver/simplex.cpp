/*
 * Created by Zhen Chen on 2025/3/8.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "simplex.h"
#include <iomanip>
#include <iostream>

constexpr double M = 10000;

// 单纯形法实现
// Bland 规则确保单纯形法不会在退化解之间循环，最终会收敛到最优解或检测到无界解
// Bland 必须同时应用到换入和换出
int Simplex::findPivotColumn() const {
  int pivotCol = -1;
  for (int j = 0; j < var_num - 1; j++) {
    // 从索引 0 开始检查
    if (tableau[0][j] < 0) {
      if (pivotCol == -1 || j < pivotCol) {
        pivotCol = j; // 选择索引最小的负检验数
      }
    }
  }
  return pivotCol;
}

// Bland 规则确保单纯形法不会在退化解之间循环，最终会收敛到最优解或检测到无界解
int Simplex::findPivotRow(const int pivotCol) const {
  int pivotRow = -1;
  double minRatio = std::numeric_limits<double>::max();
  for (int i = 1; i < con_num; i++) {
    if (tableau[i][pivotCol] > 1e-6) {
      // 避免数值误差
      const double ratio = tableau[i][var_num - 1] / tableau[i][pivotCol];
      if (ratio < minRatio) {
        // 严格小于，确保唯一性
        minRatio = ratio;
        pivotRow = i;
      } else if (abs(ratio - minRatio) < 1e-6) {
        // 比率相等时选最小索引
        if (basicVars[i - 1] < basicVars[pivotRow - 1]) {
          pivotRow = i;
        }
      }
    }
  }
  return pivotRow;
}

void Simplex::pivot(const int pivotRow, int pivotCol) {
  const double pivotValue = tableau[pivotRow][pivotCol];
  for (int j = 0; j < var_num; j++) {
    tableau[pivotRow][j] /= pivotValue;
  }
  for (int i = 0; i < con_num; i++) {
    if (i != pivotRow) {
      const double factor = tableau[i][pivotCol];
      for (int j = 0; j < var_num; j++) {
        tableau[i][j] -= factor * tableau[pivotRow][j];
      }
    }
  }
  // 更新基变量：新进入变量替换旧基变量
  basicVars[pivotRow - 1] = pivotCol;
}

Simplex::Simplex(const std::vector<std::vector<double>> &initialTableau) {
  tableau = initialTableau;
  con_num = static_cast<int>(tableau.size());
  var_num = static_cast<int>(tableau[0].size());
  initializeBasicVariables(); // 初始化基变量
}

void Simplex::solve() {
  displayTableau();
  initializeObjective();
  while (true) {
    const int pivotCol = findPivotColumn();
    if (pivotCol == -1)
      break; // 已达到最优解
    const int pivotRow = findPivotRow(pivotCol);
    if (pivotRow == -1) {
      std::cout << "无界解" << std::endl;
      return;
    }
    pivot(pivotRow, pivotCol);
  }
  displaySolution();
}

// 判断某列是否为基变量，并返回对应的行（如果不是基变量，返回 -1）
int Simplex::isBasicVariable(const int var_num) const {
  int basicRow = -1;
  for (int i = 1; i < con_num; i++) {
    if (abs(tableau[i][var_num] - 1.0) < 1e-6) {
      // 检查是否为 1
      if (basicRow == -1) {
        basicRow = i;
      } else {
        return -1; // 出现多个 1，非基变量
      }
    } else if (abs(tableau[i][var_num]) > 1e-6) {
      // 检查是否有非 0 值
      return -1; // 非 0 非 1，非基变量
    }
  }
  return basicRow; // 返回对应的基行
}

void Simplex::displaySolution() const {
  std::cout << "最优值: " << -tableau[0][var_num - 1] << std::endl;
  std::cout << "最终基变量及其值:\n";
  for (int i = 0; i < basicVars.size(); i++) {
    const int var_num = basicVars[i];
    std::cout << "x" << (var_num + 1) << " = " << tableau[i + 1][var_num - 1] << std::endl;
  }
  for (int j = 0; j < var_num - 1; j++) {
    if (find(basicVars.begin(), basicVars.end(), j) == basicVars.end()) {
      std::cout << "x" << (j + 1) << " = 0" << std::endl;
    }
  }
}

// 初始化基变量
void Simplex::initializeBasicVariables() {
  basicVars.resize(con_num - 1, -1);
  for (int i = 1; i < con_num; i++) {
    for (int j = 0; j < var_num - 1; j++) {
      if (isBasicVariable(j) == i) {
        basicVars[i - 1] = j;
        break;
      }
    }
  }
}

// 初始化目标函数（消除人工变量的M项）
void Simplex::initializeObjective() {
  for (int j = 0; j < var_num - 1; j++) {
    if (tableau[0][j] == -M) {
      // 识别人工变量列
      for (int i = 1; i < con_num; i++) {
        if (tableau[i][j] == 1) {
          // 该人工变量是基变量
          constexpr double factor = M;
          for (int k = 0; k < var_num; k++) {
            // 大M 法第一行通过行变换将人工变量的系数转化为0
            tableau[0][k] += factor * tableau[i][k];
          }
        }
      }
    }
  }
}

void Simplex::displayTableau() const {
  std::cout << "当前单纯形表:\n";
  for (int i = 0; i < con_num; i++) {
    for (int j = 0; j < var_num; j++) {
      std::cout << std::setw(8) << std::fixed << std::setprecision(2) << tableau[i][j];
    }
    std::cout << std::endl;
  }
  std::cout << "当前基变量: ";
  for (const int var : basicVars) {
    std::cout << "x" << (var + 1) << " ";
  }
  std::cout << std::endl << std::endl;
}

void Simplex::standardize() {
  // if objective is maximizing
  if (obj_sense != 0)
    for (size_t i = 0; i < var_num; i++) {
      obj_coe[i] = -obj_coe[i];
    }

  int unsigned_count = 0;
  for (size_t i = 0; i < var_num; i++) {
    switch (var_sign[i]) {
    case 1: // <= 0
      obj_coe[i] = -obj_coe[i];
      for (int j = 0; j < con_num; j++)
        con_lhs[j][i] = -con_lhs[j][i];
      break;
    case 2: {
      // unsigned
      auto it1 = obj_coe.begin();
      std::advance(it1, i + 1); // 免去对 i 的类型转换
      obj_coe.insert(it1, -obj_coe[i]);
      var_sign[i] = 0;
      auto it3 = var_sign.begin();
      std::advance(it3, i + 1); // 免去对 i 的类型转换
      var_sign.insert(it3, 0);
      for (int j = 0; j < con_num; j++) {
        auto it2 = con_lhs[j].begin();
        std::advance(it2, i + 1); // 免去对 i 的类型转换
        con_lhs[j].insert(it2, -con_lhs[j][i]);
      }
      unsigned_count++;
      break;
    }
    default:
      break;
    }
  }
  var_num += unsigned_count;

  for (size_t j = 0; j < con_num; j++) {
    if (con_rhs[j] < 0) {
      con_rhs[j] = -con_rhs[j];
      for (auto &item : con_lhs[j]) {
        item = -item; // 注意要用引用才能修改原数组
      }
      if (con_sense[j] != 0)
        con_sense[j] = -con_sense[j];
    }

    switch (con_sense[j]) {
    case 0: // <= 0
      con_slack_coe.push_back(1);
      con_artificial_coe.push_back(0);
      break;
    case 1: // >= 0
      con_slack_coe.push_back(-1);
      con_artificial_coe.push_back(1);
      break;
    default:
      con_slack_coe.push_back(0);
      con_artificial_coe.push_back(1);
      break;
    }

    con_sense[j] = 2; // making all the constraints equal
  }

  for (size_t i = 0; i < con_slack_coe.size(); i++) {
    if (con_slack_coe[i] != 0) {
      obj_coe.push_back(0);
      for (size_t j = 0; j < con_num; j++) {
        if (j == i)
          con_lhs[j].push_back(con_slack_coe[i]);
        else
          con_lhs[j].push_back(0);
      }
    }
  }

  for (size_t i = 0; i < con_artificial_coe.size(); i++) {
    if (con_artificial_coe[i] != 0) {
      obj_coe.push_back(0);
      for (size_t j = 0; j < con_num; j++) {
        if (j == i)
          con_lhs[j].push_back(con_artificial_coe[i]);
        else
          con_lhs[j].push_back(0);
      }
    }
  }
}

void Simplex::print() const {
  if (obj_sense == 0)
    std::cout << "min    ";
  else
    std::cout << "max    ";
  for (int i = 0; i < var_num; i++) {
    std::cout << obj_coe[i];
    std::cout << "x_" << (i + 1) << " ";
    if (i != var_num - 1 && obj_coe[i] >= 0)
      std::cout << "+ ";
  }
  for (size_t i = 0; i < con_slack_coe.size(); i++) {
    if (con_slack_coe[i] != 0) {
      std::cout << " + ";
      std::cout << "0s_" << (i + 1) << " ";
    }
  }
  for (size_t i = 0; i < con_artificial_coe.size(); i++) {
    if (con_artificial_coe[i] != 0) {
      std::cout << " + ";
      std::cout << "0a_" << (i + 1) << " ";
    }
  }
  std::cout << std::endl;
  std::cout << "s.t." << std::endl;
  for (int j = 0; j < con_num; j++) {
    for (size_t i = 0; i < var_num; i++) {
      if (con_lhs[j][i] != 1)
        std::cout << con_lhs[j][i];
      std::cout << "x_" << (i + 1);
      if (i != var_num - 1 && con_lhs[j][i] >= 0)
        std::cout << " + ";
    }
    if (!con_slack_coe.empty()) {
      for (size_t k = 0; k < con_slack_coe.size(); k++)
        if (k == j)
          std::cout << " + s_" << (k + 1);
        else
          std::cout << " + 0s_" << (k + 1);
    }
    switch (con_sense[j]) {
    case 0:
      std::cout << " <= ";
      break;
    case 1:
      std::cout << " >= ";
      break;
    default:
      std::cout << " = ";
      break;
    }
    std::cout << con_rhs[j] << std::endl;
  }
  for (int i = 0; i < var_num; i++) {
    if (var_sign[i] != 2 && i != 0)
      std::cout << ", ";
    if (var_sign[i] == 0)
      std::cout << "x_" << (i + 1) << " >= 0";
    if (var_sign[i] == 1)
      std::cout << "x_" << (i + 1) << " <= 0";
  }
  for (size_t i = 0; i < con_slack_coe.size(); i++) {
    if (con_slack_coe[i] != 0) {
      std::cout << ", ";
      std::cout << "s_" << (i + 1) << " >= 0";
    }
  }
  for (size_t i = 0; i < con_artificial_coe.size(); i++) {
    if (con_artificial_coe[i] != 0) {
      std::cout << ", ";
      std::cout << "a_" << (i + 1) << " >= 0";
    }
  }
  std::cout << std::endl;
}

int main() {
  // 初始化单纯形表
  // 标准化的单纯性表，目标函数为 max
  // 目标函数: max z = 2x1 + 3x2 转换为 z -2x1 - 3x2
  // 约束: 2x1 + x2 + s1 = 4
  //       x1 + 2x2 + s2 = 5

  constexpr int obj_sense = 1;
  const std::vector obj_coe = {2.0, 3.0};
  const std::vector<std::vector<double>> con_lhs = {{2.0, 1.0}, {1.0, 2.0}};
  const std::vector con_rhs = {4.0, 5.0};
  const std::vector con_sense = {0, 0}; // 0:<=, 1: >=, 2: =
  const std::vector var_sign = {2, 2};  // 0: >=, 1: <=, 2: unsigned

  auto model = Simplex(obj_sense, obj_coe, con_lhs, con_rhs, con_sense, var_sign);
  model.print();

  model.standardize();
  std::cout << "***************************************" << std::endl;
  model.print();

  // const std::vector<std::vector<double>> tableau = {
  //     {-2, -3, 0, 0, 0}, // 目标函数 z -2x1 - 3x2
  //     {2, 1, 1, 0, 4},   // 约束1
  //     {1, 2, 0, 1, 5}    // 约束2
  // };
  //
  // Simplex simplex(tableau);
  // simplex.solve();
  // std::cout << "****************************" << std::endl;

  // // 初始化单纯形表
  // //  目标函数: max z = 2x1 + 3x2 转换为 -2x1 - 3x2 + M*a1 + M*a2 + z = 0
  // //  约束: x1+x2 >= 2, i.e.,  x1 + x2 - s1 + a1 = 2
  // //       2x1+x2 = 4, i.e., 2x1 + x2 + a2 = 4
  // const std::vector<std::vector<double>> tableau2 = {
  //     {-2, -3, 1, -M, -M, 0}, // 目标函数 (x1, x2, s1, a1, a2, z)
  //     {1, 1, -1, 1, 0, 2},    // 约束1
  //     {2, 1, 0, 0, 1, 4}      // 约束2
  // };
  // Simplex simplex2(tableau2);
  // simplex2.solve();
  //
  // std::cout << "****************************" << std::endl;
  // // test recycling
  // // maximize z = (3/4)x1 -20x2 + (1/2)x3 -6x4 subject to
  // // (1 / 4)x1 - 8x2 - x3 + 9x4 <= 0
  // // (1 / 2)x1 - 12x2 - (1 / 2)x3 + 3x4 <= 0
  // // x3 <= 1
  // const std::vector<std::vector<double>> tableau3 = {
  //     {-3.0 / 4, 20, -1.0 / 2, 6, 0, 0, 0, 0}, // 目标函数
  //     {1.0 / 4, -8, -1, 9, 1, 0, 0, 0},        // 约束1
  //     {1.0 / 2, -12, -1.0 / 2, 3, 0, 1, 0, 0}, // 约束2
  //     {0, 0, 1, 0, 0, 0, 1, 1}                 // 约束3
  // };
  //
  // Simplex simplex3(tableau3);
  // simplex3.solve();

  return 0;
}
