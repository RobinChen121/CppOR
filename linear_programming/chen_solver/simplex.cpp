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
#include <numeric>

// #include <emscripten/bind.h>
// using namespace emscripten;
//
// // 使用 Emscripten 绑定暴露 Simplex 类和 solve 函数
// // 必须注册才能调用
// EMSCRIPTEN_BINDINGS(simplex_module) {
//   // 注册 vector 类型，相当于在js中重新定义了几个数据类型
//   register_vector<double>("VectorDouble");
//   register_vector<int>("VectorInt");
//   register_vector<std::vector<double>>("VectorVectorDouble");
//
//   // 注册类
//   class_<Simplex>("Simplex")
//       .constructor<int, std::vector<double>, std::vector<std::vector<double>>,
//       std::vector<double>,
//                    std::vector<int>, std::vector<int>>()
//       // 注册类里面的函数
//       // 前面的字符串名字是 js 里面使用的名字
//       .function("testWeb", &Simplex::testWeb)
//       .function("solve", &Simplex::solve)
//       .function("standardize", &Simplex::standardize)
//       .function("getStatus", &Simplex::getStatus)
//       .function("getOptValue", &Simplex::getOptValue)
//       .function("getOptSolution", &Simplex::getOptSolution);
// }

constexpr double M = 10000;
// 单纯形法实现
// Bland 规则确保单纯形法不会在退化解之间循环，最终会收敛到最优解或检测到无界解
// Bland 必须同时应用到换入和换出
int Simplex::findPivotColumn() const {
  int pivot_column = -1;
  double min_value = std::numeric_limits<double>::max();
  for (int j = 0; j < var_total_num; j++) {
    if (anti_cycle == AntiCycle::Bland) {
      // 从索引 0 开始检查
      if (tableau[0][j] < -1e-6) {
        if (pivot_column == -1 || j < pivot_column) {
          pivot_column = j; // 选择索引最小的负检验数
        }
      }
    }
    if (anti_cycle == AntiCycle::None) {
      if (tableau[0][j] < 0 and tableau[0][j] < min_value) {
        pivot_column = j;
        min_value = tableau[0][j];
      }
    }
  }
  return pivot_column;
}

// Bland 规则确保单纯形法不会在退化解之间循环，最终会收敛到最优解或检测到无界解
int Simplex::findPivotRow(const int pivot_column) const {
  int pivot_row = -1;
  double minRatio = std::numeric_limits<double>::max();
  for (int i = 1; i < constraint_num + 1; i++) {
    if (tableau[i][pivot_column] > 1e-6) {
      // 避免数值误差
      const double ratio = tableau[i][var_total_num] / tableau[i][pivot_column];
      if (ratio < minRatio) {
        // 严格小于，确保唯一性
        minRatio = ratio;
        pivot_row = i;
      } else if (abs(ratio - minRatio) < 1e-6) {
        // 比率相等时选最小索引
        if (basic_vars[i - 1] < basic_vars[pivot_row - 1]) {
          pivot_row = i;
        }
      }
    }
  }
  return pivot_row;
}

void Simplex::pivot(const int pivot_row, const int pivot_column) {
  const double pivot_value = tableau[pivot_row][pivot_column];
  for (int j = 0; j < var_total_num + 1; j++) {
    tableau[pivot_row][j] /= pivot_value; // 除以 pivot_value
  }
  for (int i = 0; i < constraint_num + 1; i++) {
    if (i != pivot_row) {
      const double factor = tableau[i][pivot_column];
      for (int j = 0; j < var_total_num + 1; j++) {
        tableau[i][j] -= factor * tableau[pivot_row][j];
      }
    }
  }
  // 更新基变量：新进入变量替换旧基变量
  basic_vars[pivot_row - 1] = pivot_column;
}

double Simplex::computeReduceCost(const std::vector<double> &cB, const int Ai_index) const {
  double product = 0.0;
  for (size_t i = 0; i < cB.size(); i++) {
    product += -cB[i] * tableau[i + 1][Ai_index];
  }
  return product;
}

// NOLINTNEXTLINE(misc-no-recursion)
void Simplex::solve() {
  // initializeObjective(); // change the big M variables

  // two-stage
  if (var_artificial_num > 1e-6) {
    // fist-stage
    std::vector<double> new_obj_coe1(var_total_num);
    for (int i = var_total_num - var_artificial_num; i < var_total_num; i++)
      new_obj_coe1[i] = 1.0;

    // get CB and compute the new reduced cost
    std::vector<double> CB1(basic_vars.size());
    for (auto i : basic_vars)
      CB1[i] = new_obj_coe1[i];
    std::vector<double> reduced_costs1(var_total_num + 1);
    for (int i = 0; i < var_total_num; i++)
      reduced_costs1[i] = new_obj_coe1[i] + computeReduceCost(CB1, i);
    reduced_costs1[var_total_num] = computeReduceCost(CB1, var_total_num);

    auto original_reduced_costs = tableau[0];
    tableau[0] = reduced_costs1;
    Simplex simplex1(tableau);

    simplex1.solve();
    // auto solution = simplex1.getOptSolution();
    auto value = simplex1.getOptValue();
    simplex1.printTableau();
    if (!value.has_value() or value.value() > 1e-6) {
      solution_status = 2;
      std::cout << std::string(50, '*') << std::endl;
      std::cout << "the problem is infeasible" << std::endl;
      return;
    }
    bool all_zeros = false;
    int row_index = -1;
    while (true) {
      basic_vars = simplex1.basic_vars;
      row_index = artificialIndexInBasicVars();
      if (row_index == -1 or all_zeros)
        break;
      all_zeros = true;
      for (int i = 0; i < var_total_num - var_artificial_num - 1; i++) {
        if (std::abs(simplex1.tableau[row_index][i]) > 1e-6) {
          simplex1.pivot(row_index, i);
          all_zeros = false;
          break;
        }
      }
    }
    simplex1.printTableau();
    std::vector<double> new_obj_coe2(var_total_num - var_artificial_num);
    for (int i = 0; i < var_total_num - var_artificial_num; i++)
      new_obj_coe2[i] = obj_coe[i];

    // get CB and compute the new reduced cost
    tableau = simplex1.tableau;
    std::vector<double> CB2(basic_vars.size());
    for (auto i : basic_vars)
      CB2[i] = new_obj_coe2[i];
    std::vector<double> reduced_costs2(var_total_num - var_artificial_num + 1);
    for (int i = 0; i < var_total_num - var_artificial_num; i++)
      reduced_costs2[i] = new_obj_coe2[i] + computeReduceCost(CB2, i);
    reduced_costs2[var_total_num - var_artificial_num] = computeReduceCost(CB2, var_total_num);
    std::vector<int> artificial_column(var_artificial_num);
    for (int i = 0; i < var_artificial_num; i++)
      artificial_column[i] = var_original_num + var_slack_num + i;

    eraseColumns(tableau, artificial_column);
    tableau[0] = reduced_costs2;
    if (all_zeros) {
      tableau.erase(tableau.begin() + row_index);
      basic_vars.erase(basic_vars.begin() + row_index - 1);
    }
    // second-stage computation

    printTableau();
    Simplex simplex2(tableau);
    simplex2.solve();
    tableau = simplex2.tableau;
    var_total_num -= var_artificial_num;
    var_artificial_num = 0;
    displaySolution();
  }

  while (true) {
    const int pivot_column = findPivotColumn();
    if (pivot_column == -1) {
      solution_status = 0; // revise
      break;               // 已达到最优解
    }
    const int pivot_row = findPivotRow(pivot_column);
    if (pivot_row == -1) {
      std::cout << "unbounded" << std::endl;
      solution_status = 1;
    }
    // pivot row and pivot column are the index in the tableau
    pivot(pivot_row, pivot_column);
  }
  // displaySolution();
}

// 判断某列是否为基变量，并返回对应的行（如果不是基变量，返回 -1）
int Simplex::isBasicVariable(const int column_index) const {
  int basicRow = -1;
  for (int i = 1; i < constraint_num + 1; i++) {
    if (abs(tableau[i][column_index] - 1.0) < 1e-6) {
      // 检查是否为 1
      if (basicRow == -1) {
        basicRow = i - 1;
      } else {
        return -1; // 出现多个 1，非基变量
      }
    } else if (abs(tableau[i][column_index]) > 1e-6) {
      // 检查是否有非 0 值
      return -1; // 非 0 非 1，非基变量
    }
  }
  return basicRow; // 返回对应的基行
}

std::optional<double> Simplex::getOptValue() const {
  if (solution_status == 0) {
    const double opt_value =
        obj_sense_changed ? tableau[0][var_total_num] : -tableau[0][var_total_num];
    return opt_value;
  }
  return std::nullopt;
}

std::optional<std::vector<double>> Simplex::getOptSolution() const {
  if (solution_status == 0) {
    auto basic_v = basic_vars;
    std::ranges::sort(basic_v);
    std::vector<double> solution(var_total_num);
    for (int i = 0; i < basic_v.size(); i++) {
      const int var_index = basic_v[i];
      solution[var_index] = tableau[i + 1][var_total_num];
    }
    return solution;
  }
  return std::nullopt;
}

void Simplex::displaySolution() const {
  if (solution_status == 0) {
    const double opt_value =
        obj_sense_changed ? tableau[0][var_total_num] : -tableau[0][var_total_num];
    std::vector<int> indices(basic_vars.size());
    std::iota(indices.begin(), indices.end(), 0); // fill the range sequentially with a start value
    // there is a lambda function below
    // class attribute vectors should use [this], not [&basic_vars]
    std::ranges::sort(
        indices, [this](const int i1, const int i2) { return basic_vars[i1] < basic_vars[i2]; });

    std::cout << std::string(50, '*') << std::endl;
    std::cout << "The optimal value is: " << opt_value << std::endl;
    std::cout << std::endl;
    std::cout << "The final basic variables and the corresponding values:\n";
    for (int i = 0; i < basic_vars.size(); i++) {
      const int var_index = basic_vars[indices[i]];
      if (var_index <= var_original_num - 1)
        std::cout << "x" << (var_index + 1) << " = " << tableau[indices[i] + 1][var_total_num]
                  << std::endl;
      else if (var_index <= var_original_num + var_slack_num - 1)
        std::cout << "s" << (var_index - var_original_num + 1) << " = "
                  << tableau[indices[i] + 1][var_total_num] << std::endl;
      else
        std::cout << "a" << (var_index - var_original_num - var_artificial_num + 1) << " = "
                  << tableau[indices[i] + 1][var_total_num] << std::endl;
    }
  }
}

// 初始化基变量
void Simplex::initializeBasicVariables() {
  basic_vars.resize(constraint_num, -1);
  for (int i = 0; i < constraint_num; i++) {
    for (int j = 0; j < var_total_num; j++) {
      if (isBasicVariable(j) == i) {
        basic_vars[i] = j;
        break;
      }
    }
  }
}

// 初始化目标函数（消除人工变量的M项）
void Simplex::initializeObjective() {
  for (int j = 0; j < var_total_num - 1; j++) {
    if (tableau[0][j] == -M) {
      // 识别人工变量列
      for (int i = 1; i < constraint_num; i++) {
        if (tableau[i][j] == 1) {
          // 该人工变量是基变量
          constexpr double factor = M;
          for (int k = 0; k < var_total_num; k++) {
            // 大M 法第一行通过行变换将人工变量的系数转化为0
            tableau[0][k] += factor * tableau[i][k];
          }
        }
      }
    }
  }
}

void Simplex::standardize() {
  // if objective is maximizing
  if (obj_sense != 0) {
    for (size_t i = 0; i < var_total_num; i++) {
      obj_coe[i] = -obj_coe[i];
    }
    obj_sense = 0;
    obj_sense_changed = true;
  }

  for (size_t i = 0; i < var_total_num; i++) {
    switch (var_sign[i]) {
    case 1: // <= 0
      obj_coe[i] = -obj_coe[i];
      for (int j = 0; j < constraint_num; j++)
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
      for (int j = 0; j < constraint_num; j++) {
        auto it2 = con_lhs[j].begin();
        std::advance(it2, i + 1); // 免去对 i 的类型转换
        con_lhs[j].insert(it2, -con_lhs[j][i]);
      }
      i++;
      var_total_num++;
      break;
    }
    default:
      break;
    }
  }

  for (size_t j = 0; j < constraint_num; j++) {
    if (con_rhs[j] < 0) {
      con_rhs[j] = -con_rhs[j];
      for (auto &item : con_lhs[j]) {
        item = -item; // 注意要用引用才能修改原数组
      }
      if (constraint_sense[j] != 0)
        constraint_sense[j] = -constraint_sense[j];
    }

    switch (constraint_sense[j]) {
    case 0: // <= 0
      con_slack_coe.push_back(1);
      con_artificial_coe.push_back(0);
      break;
    case 1: // >= 0
      con_slack_coe.push_back(-1);
      con_artificial_coe.push_back(1);
      break;
    default: // ==
      con_slack_coe.push_back(0);
      bool only_one = true; // 判断是否存在已有的变量可以作为初始基变量
      for (size_t i = 0; i < var_total_num; i++) {
        if (i == j) {
          if (std::abs(con_lhs[j][i] - 1.0) > 1e-6) {
            only_one = false;
            break;
          }
        } else if (std::abs(con_lhs[j][i]) > 1e-6) {
          only_one = false;
          break;
        }
      }
      if (only_one)
        con_artificial_coe.push_back(1);
      break;
    }

    constraint_sense[j] = 2; // making all the constraints equal
  }

  for (size_t i = 0; i < con_slack_coe.size(); i++) {
    if (con_slack_coe[i] != 0) {
      obj_coe.push_back(0);
      var_slack_num++;
      for (size_t j = 0; j < constraint_num; j++) {
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
      var_artificial_num++;
      for (size_t j = 0; j < constraint_num; j++) {
        if (j == i)
          con_lhs[j].push_back(con_artificial_coe[i]);
        else
          con_lhs[j].push_back(0);
      }
    }
  }

  // initialize tableau
  var_total_num += var_artificial_num + var_slack_num;
  tableau.resize(constraint_num + 1);
  for (size_t i = 0; i <= constraint_num; i++) {
    tableau[i].resize(var_total_num + 1);
    if (i == 0) {
      for (size_t j = 0; j < var_total_num; j++)
        tableau[i][j] = obj_coe[j];
      tableau[i][var_total_num] = 0;
    } else {
      for (size_t j = 0; j < var_total_num; j++)
        tableau[i][j] = con_lhs[i - 1][j];
      tableau[i][var_total_num] = con_rhs[i - 1];
    }
  }
  initializeBasicVariables();
}

void Simplex::print() const {
  const int var_original_num = var_total_num - var_slack_num - var_artificial_num;
  if (obj_sense == 0)
    std::cout << "min    ";
  else
    std::cout << "max    ";
  for (int i = 0; i < var_original_num; i++) {
    if (std::abs(-1 - obj_coe[i]) < 1e-6)
      std::cout << "- ";
    else if (std::abs(1 - obj_coe[i]) > 1e-6)
      std::cout << obj_coe[i];
    std::cout << "x_" << (i + 1) << " ";
    if (i != obj_coe.size() - 1 && obj_coe[i + 1] > -1e-6)
      std::cout << "+ ";
  }
  for (size_t i = 0; i < con_slack_coe.size(); i++) {
    if (std::abs(con_slack_coe[i]) > 1e-6) {
      if (i != 0)
        std::cout << "+ ";
      std::cout << "0s_" << (i + 1);
    }
  }
  int artificial_count = 0;
  for (const int i : con_artificial_coe) {
    if (i != 0) {
      std::cout << " + ";
      std::cout << "0a_" << (artificial_count + 1) << "";
      artificial_count++;
    }
  }
  std::cout << std::endl;
  std::cout << "s.t." << std::endl;
  for (int j = 0; j < constraint_num; j++) {
    for (size_t i = 0; i < var_original_num; i++) {
      if (std::abs(-1 - con_lhs[j][i]) < 1e-6)
        std::cout << "-";
      else if (std::abs(1 - con_lhs[j][i]) > 1e-6 && con_lhs[j][i] > -1e-6)
        std::cout << con_lhs[j][i];
      else if (std::abs(1 - con_lhs[j][i]) > 1e-6 && con_lhs[j][i] < -1e-6)
        std::cout << "-" << -con_lhs[j][i];
      std::cout << "x_" << (i + 1);
      if (i != var_original_num - 1 && con_lhs[j][i + 1] > -1e-6)
        std::cout << " + ";
      if (i != var_original_num - 1 && con_lhs[j][i + 1] < -1e-6)
        std::cout << " ";
    }
    if (var_slack_num > 1e-1) {
      int slack_count = 0;
      for (size_t k = 0; k <= j; k++) {
        if (std::abs(con_slack_coe[k]) > 1e-6)
          slack_count++;
      }
      if (std::abs(con_slack_coe[j]) > 1e-6) {
        for (int m = 0; m < slack_count - 1; m++)
          std::cout << " + 0s_" << (m + 1);
        if (con_slack_coe[j] == 1)
          std::cout << " + s_" << (slack_count);
        else if (std::abs(-1 - con_slack_coe[j]) < 1e-6)
          std::cout << " -s_" << (slack_count);
        for (int m = slack_count + 1; m <= var_slack_num; m++)
          std::cout << " + 0s_" << (m);
      } else {
        for (int m = 0; m < var_slack_num; m++)
          std::cout << " + 0s_" << (m + 1);
      }
    }
    if (var_artificial_num > 1e-1) {
      artificial_count = 0;
      for (size_t k = 0; k <= j; k++) {
        if (std::abs(con_artificial_coe[k]) > 1e-6)
          artificial_count++;
      }
      if (con_artificial_coe[j] == 1) {
        for (int m = 0; m < artificial_count - 1; m++)
          std::cout << " + 0a_" << (m + 1);
        std::cout << " + a_" << (artificial_count);
        for (int m = artificial_count + 1; m <= var_artificial_num; m++)
          std::cout << " + 0a_" << (m);
      } else {
        for (int m = 0; m < var_artificial_num; m++)
          std::cout << " + 0a_" << (m + 1);
      }
    }
    switch (constraint_sense[j]) {
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
  for (int i = 0; i < var_original_num; i++) {
    if (var_sign[i] != 2 && i != 0)
      std::cout << ", ";
    if (var_sign[i] == 0)
      std::cout << "x_" << (i + 1) << " >= 0";
    if (var_sign[i] == 1)
      std::cout << "x_" << (i + 1) << " <= 0";
  }
  for (size_t i = 0; i < con_slack_coe.size(); i++) {
    if (std::abs(con_slack_coe[i]) > 1e-6) {
      std::cout << ", ";
      std::cout << "s_" << (i + 1) << " >= 0";
    }
  }
  artificial_count = 0;
  for (const int i : con_artificial_coe) {
    if (i != 0) {
      std::cout << ", ";
      std::cout << "a_" << (artificial_count + 1) << " >= 0";
      artificial_count++;
    }
  }
  std::cout << std::endl;
}

// return -1 if no artificial variable in the basic variables, else return the row index of the
// first artificial variable in the tableau
int Simplex::artificialIndexInBasicVars() {
  for (int i = var_total_num - var_artificial_num - 1; i < var_total_num; i++) {
    if (auto it = std::ranges::find(basic_vars, i); it != basic_vars.end()) {
      const size_t index = std::distance(basic_vars.begin(), it);
      return static_cast<int>(index + 1);
    }
  }
  return -1;
}

void Simplex::setAntiCycle(const AntiCycle rule) { anti_cycle = rule; }

void Simplex::printConLHS() const {
  for (auto &row : con_lhs) {
    for (auto &col : row)
      std::cout << col << " ";
    std::cout << std::endl;
  }
}

void Simplex::printTableau() const {
  std::cout << std::string(50, '*') << std::endl;
  std::cout << "The tableau is:" << std::endl;
  for (const auto &row : tableau) {
    for (const auto column : row)
      std::cout << std::fixed << std::setprecision(2) << column << "   ";
    std::cout << std::endl;
  }
}

void Simplex::checkInput() const {
  const size_t obj_coe_num = obj_coe.size();
  const size_t con_row_num = con_lhs.size();
  const size_t con_column_num = con_lhs[0].size();
  const size_t con_sense_num = constraint_sense.size();
  const size_t var_num = var_sign.size();
  if (!(obj_coe_num == var_num && var_num == con_column_num)) {
    std::cerr << "wrong input!" << std::endl;
    exit(-1);
  }
  if (con_row_num != con_sense_num) {
    std::cerr << "wrong input!" << std::endl;
    exit(-1);
  }
}

double Simplex::testWeb() const { // NOLINT(*-convert-member-functions-to-static)
  double sum = 0.0;
  for (const auto &i : obj_coe)
    sum += i;
  return sum;
}

int main() {
  // 初始化单纯形表
  // 标准化的单纯性表，目标函数为 max
  // 目标函数: max z = 2x1 + 3x2 转换为 z -2x1 - 3x2
  // 约束: 2x1 + x2 + s1 = 4
  //       x1 + 2x2 + s2 = 5

  // constexpr int obj_sense = 1;
  // const std::vector obj_coe = {2.0, 3.0};
  // const std::vector<std::vector<double>> con_lhs = {{2, 1}, {1, 2}};
  // const std::vector con_rhs = {4.0, 5.0};
  // const std::vector constraint_sense = {0, 0};
  // const std::vector var_sign = {0, 0};

  constexpr int obj_sense = 1;
  const std::vector obj_coe = {2.0, 1.0};
  const std::vector<std::vector<double>> con_lhs = {
      {1.0, 1.0},
      {2.0, 2.0},
  };
  const std::vector con_rhs = {2.0, 6.0};
  const std::vector constraint_sense = {0, 1}; // 0:<=, 1: >=, 2: =
  const std::vector var_sign = {0, 0};         // 0: >=, 1: <=, 2: unsigned

  auto model = Simplex(obj_sense, obj_coe, con_lhs, con_rhs, constraint_sense, var_sign);
  model.checkInput();
  std::cout << "original model is:" << std::endl;
  model.print();

  model.standardize();
  std::cout << std::string(50, '*') << std::endl;
  std::cout << "the standardized model is:" << std::endl;
  model.print();
  // model.setAntiCycle(AntiCycle::Bland);
  model.solve();
  model.displaySolution();

  // const std::vector<std::vector<double>> tableau = {
  //     {-2, -3, 0, 0, 0}, // 目标函数 z -2x1 - 3x2
  //     {2, 1, 1, 0, 4},   // 约束1
  //     {1, 2, 0, 1, 5}    // 约束2
  // };
  //
  // Simplex simplex(tableau);
  // simplex.initializeBasicVariables();
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
