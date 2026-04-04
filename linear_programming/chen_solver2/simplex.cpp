/*
 * Created by Zhen Chen on 2026/3/20.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "simplex.h"

#include <algorithm>
#include <chrono>
#include <iomanip> // for precision
#include <iostream>
#include <numeric>
#include <optional>

// 转化成js时多维数组不能作为全局变量 下面 emcc 的一些命令显示红字是正常的，通过命令行生成 js，wasm
//     文件
#include <emscripten/bind.h>
using namespace emscripten;

// 使用 Emscripten 绑定暴露 Simplex 类和 solve 函数
// 必须注册才能调用
EMSCRIPTEN_BINDINGS(simplex_module) {
  // 注册 vector 类型，相当于在js中重新定义了几个数据类型
  register_vector<double>("VectorDouble");
  register_vector<int>("VectorInt");
  register_vector<std::vector<double>>("VectorVectorDouble");
  register_vector<std::vector<int>>("VectorVectorInt");
  register_vector<std::vector<std::vector<double>>>("VectorVectorVectorDouble");

  // 注册类
  class_<Simplex>("Simplex")
      .constructor<int, std::vector<double>, std::vector<std::vector<double>>, std::vector<double>,
                   std::vector<int>, std::vector<int>, int>() // 这个是类的构造函数
      // 注册类里面的函数
      // 前面的字符串名字是 js 里面使用的名字
      .function("solve", &Simplex::solve)
      .function("standardize", &Simplex::standardize)
      .function("getStatus", &Simplex::getStatus)
      .function("getOptValue", &Simplex::getOptValue)
      .function("getBasicVars", &Simplex::getBasicVarsIndices)
      .function("getRecordedTableau", &Simplex::get_recorded_tableau)
      .function("getOptSolution", &Simplex::getOptSolution)
      .function("getTime", &Simplex::getTime)
      .function("getPivotIndex", &Simplex::get_recorded_pivot);
}

// -------------------------
// non-class functions

void printCSC(const CSC &csc) {
  std::cout << "values: ";
  for (const auto v : csc.non_zero_values)
    std::cout << v << " ";
  std::cout << std::endl;

  std::cout << "row_idx: ";
  for (const auto r : csc.row_indices)
    std::cout << r << " ";
  std::cout << std::endl;

  std::cout << "col_start_end: ";
  for (const auto c : csc.col_start_end)
    std::cout << c << " ";
  std::cout << std::endl;
}

CSC denseToCSC(const std::vector<std::vector<double>> &A) {
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

double getValue(const CSC &csc, const int row, const int col) {
  for (int idx = csc.col_start_end[col]; idx < csc.col_start_end[col + 1]; ++idx) {
    if (csc.row_indices[idx] == row)
      return csc.non_zero_values[idx];
  }
  return 0.0;
}

// -------------------------
// class functions

void Simplex::standardize() {
  // if objective is maximizing
  if (obj_sense != 0) {
    for (size_t i = 0; i < var_num_start; i++) {
      obj_coe[i] = -obj_coe[i];
    }
    obj_sense = 0;
    obj_sense_changed = true;
  }

  for (size_t i = 0; i < var_num_start; i++) {
    switch (var_sign[i]) {
    case 1: // <= 0
      obj_coe[i] = -obj_coe[i];
      for (int j = 0; j < con_num; j++)
        con_lhs[j][i] = -con_lhs[j][i];
      if (i == 0)
        front_unsigned_num.emplace_back(0);
      else
        front_unsigned_num.emplace_back(front_unsigned_num.back());
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
      var_num_total++;
      var_num_unsigned++;
      if (i == 0)
        front_unsigned_num.emplace_back(0);
      else
        front_unsigned_num.emplace_back(front_unsigned_num.back());
      front_unsigned_num.emplace_back(var_num_unsigned);
      i++;
      break;
    }
    default: // >= 0
      if (i == 0)
        front_unsigned_num.emplace_back(0);
      else
        front_unsigned_num.emplace_back(front_unsigned_num.back());
      break;
    }
  }

  for (size_t j = 0; j < con_num; j++) {
    if (con_rhs[j] < -eps) {
      con_rhs[j] = -con_rhs[j];
      for (auto &item : con_lhs[j]) {
        item = -item; // 注意要用引用才能修改原数组
      }
      if (constraint_sense[j] != 2)
        constraint_sense[j] = constraint_sense[j] == 1 ? 0 : 1;
    }

    switch (constraint_sense[j]) {
    case 0: // <= 0
      con_coe_slack.emplace_back(1);
      con_coe_artificial.emplace_back(0);
      break;
    case 1: // >= 0
      con_coe_slack.emplace_back(-1);
      con_coe_artificial.emplace_back(1);
      break;
    default: // == 等号时原则要在约束条件里加上一个人工变量
      con_coe_slack.emplace_back(0);
      bool exist_basic_var = true; // 判断是否存在已有的变量可以作为初始基变量
      for (size_t i = 0; i < var_num_total; i++) {
        exist_basic_var = true;
        if (std::abs(con_lhs[j][i] - 1.0) > eps) {
          exist_basic_var = false;
        } else {
          for (size_t k = 0; k < con_num; k++)
            if (k != j and std::abs(con_lhs[k][i]) > eps) {
              exist_basic_var = false;
              break;
            }
        }
        if (exist_basic_var)
          break;
      }
      if (!exist_basic_var)
        con_coe_artificial.emplace_back(1);
      else
        con_coe_artificial.emplace_back(0);
      break;
    }
    constraint_sense[j] = 2; // making all the constraints equal
  }

  for (size_t i = 0; i < con_coe_slack.size(); i++) {
    if (con_coe_slack[i] != 0) {
      obj_coe.emplace_back(0);
      var_num_slack++;
      for (size_t j = 0; j < con_num; j++) {
        if (j == i)
          con_lhs[j].emplace_back(con_coe_slack[i]);
        else
          con_lhs[j].emplace_back(0);
      }
    }
  }

  for (size_t i = 0; i < con_coe_artificial.size(); i++) {
    if (con_coe_artificial[i] != 0) {
      obj_coe.emplace_back(0);
      var_num_artificial++;
      for (size_t j = 0; j < con_num; j++) {
        if (j == i)
          con_lhs[j].emplace_back(con_coe_artificial[i]);
        else
          con_lhs[j].emplace_back(0);
      }
    }
  }

  // initialize tableau
  var_num_total += var_num_artificial + var_num_slack;
  tableau.resize(con_num + 1);
  for (size_t i = 0; i <= con_num; i++) {
    tableau[i].resize(var_num_total + 1);
    if (i == 0) {
      for (size_t j = 0; j < var_num_total; j++)
        tableau[i][j] = obj_coe[j];
      tableau[i][var_num_total] = 0;
    } else {
      for (size_t j = 0; j < var_num_total; j++)
        tableau[i][j] = con_lhs[i - 1][j];
      tableau[i][var_num_total] = con_rhs[i - 1];
    }
  }
  initializeBasicVars();
}

void Simplex::initializeBasicVars() {
  basic_var_index.resize(con_num, -1);
  for (int i = 0; i < con_num; i++) {
    for (int j = 0; j < var_num_total; j++) {
      if (isBasicVar(j) == i) {
        basic_var_index[i] = j;
        break;
      }
    }
  }
}

// 判断某列是否为基变量，并返回对应的行（如果不是基变量，返回 -1）
int Simplex::isBasicVar(const int column_index) const {
  int basicRow = -1;
  for (int i = 1; i < con_num + 1; i++) {
    if (abs(tableau[i][column_index] - 1.0) < eps) {
      // 检查是否为 1
      if (basicRow == -1) {
        basicRow = i - 1;
      } else {
        return -1; // 出现多个 1，非基变量
      }
    } else if (abs(tableau[i][column_index]) > eps) {
      // 检查是否有非 0 值
      return -1; // 非 0 非 1，非基变量
    }
  }
  return basicRow; // 返回对应的基行
}

void Simplex::solve() {
  const auto start_time = std::chrono::high_resolution_clock::now();
  if (var_num_artificial > eps) {
    firstStage();
  } else
    singleStage();
  const auto end_time = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> diff = end_time - start_time;
  run_time = diff.count();
}

void Simplex::firstStage() {
  std::vector<double> obj_coe_stage1(var_num_total);
  for (int i = var_num_total - var_num_artificial; i < var_num_total; i++)
    obj_coe_stage1[i] = 1.0;

  // compute the new reduced cost
  const std::vector<double> reduced_cost_stage1 = computeReduceCostAll(obj_coe_stage1);

  tableau[0] = reduced_cost_stage1;
  // if (bool_record_tableau)
  //   recorded_tableau.emplace_back(tableau);

  singleStage();

  if (anti_cycle_rule == 0 and solution_status == 3)
    return;

  const auto value = getOptValue();
  // printTableau();
  if (!value.has_value() or value.value() > eps) {
    solution_status = 2;
    std::cout << std::string(50, '*') << std::endl;
    std::cout << "the problem is infeasible" << std::endl;
    return;
  }
  while (true) { // erase all the basic rows that has artificial variable
    const int row_index =
        artificialIndexInBasicVars(); // whether artificial variable is still in the basics
    if (row_index != -1) {
      // printTableau();
      tableau.erase(tableau.begin() + row_index);
      basic_var_index.erase(basic_var_index.begin() + row_index - 1);
      con_num--;
      // var_num_total -= 1;
    } else
      break;
  }

  std::vector<double> obj_coe_stage2(var_num_total - var_num_artificial);
  for (int i = 0; i < var_num_total - var_num_artificial; i++)
    obj_coe_stage2[i] = obj_coe[i];

  // erase the columns of artificial variables
  std::vector<int> artificial_column_index(var_num_artificial);
  for (int i = 0; i < var_num_artificial; i++)
    artificial_column_index[i] = var_num_total - var_num_artificial + i;
  eraseColumns(tableau, artificial_column_index);
  var_num_total -= var_num_artificial;

  // get CB and compute the new reduced cost
  const std::vector<double> reduced_cost_stage2 = computeReduceCostAll(obj_coe_stage2);
  tableau[0] = reduced_cost_stage2;

  if (anti_cycle_rule == 0 and bool_record_tableau) {
    recorded_basic_var_index.clear();
  }

  // second-stage computation
  // printTableau();
  solution_status = 0;
  // if (bool_record_tableau)
  //   recorded_tableau.emplace_back(tableau);
  singleStage();
  var_num_artificial = 0;
}

// compute the reduced cost for all variables
std::vector<double> Simplex::computeReduceCostAll(const std::vector<double> &obj_coe_) const {
  std::vector<double> cB;
  const int this_var_num = obj_coe_.size();
  cB.reserve(basic_var_index.size());
  for (const int idx : basic_var_index)
    cB.emplace_back(obj_coe_[idx]);
  std::vector<double> reduced_cost_all(this_var_num + 1);
  for (int i = 0; i < this_var_num; i++)
    reduced_cost_all[i] = obj_coe_[i] + computeReduceCostSingle(cB, i);
  reduced_cost_all[this_var_num] = computeReduceCostSingle(cB, this_var_num);
  return reduced_cost_all;
}

// compute the reduced cost for one variable
double Simplex::computeReduceCostSingle(const std::vector<double> &cB, const int Ai_index) const {
  double product = 0.0;
  for (size_t i = 0; i < cB.size(); i++) {
    product += -cB[i] * tableau[i + 1][Ai_index];
  }
  return product;
}

// return -1 if no artificial variable in the basic variables, else return the row index of the
// first artificial variable in the tableau
int Simplex::artificialIndexInBasicVars() {
  for (int i = var_num_total - var_num_artificial; i < var_num_total; i++) {
    if (auto it = std::ranges::find(basic_var_index, i); it != basic_var_index.end()) {
      const size_t index = std::distance(basic_var_index.begin(), it);
      return static_cast<int>(index + 1);
    }
  }
  return -1;
}

void Simplex::singleStage() {
  while (true) {
    if (bool_record_tableau)
      recorded_tableau.emplace_back(tableau);
    const int pivot_column = findPivotColumn();
    if (pivot_column == -1) {
      solution_status = 0;
      return;
    }
    const int pivot_row = findPivotRow(pivot_column);
    if (pivot_row == -1) {
      printTableau();
      std::cout << std::string(50, '*') << std::endl;
      std::cout << "the problem is unbounded" << std::endl;
      solution_status = 1;
      return;
    }

    if (anti_cycle_rule == 0) {
      if (!recorded_basic_var_index.empty() and
          detectCycling(basic_var_index, recorded_basic_var_index)) {
        std::cout << std::string(50, '*') << std::endl;
        std::cout << "the pivoting rule is cycling" << std::endl;
        solution_status = 3;
        return;
      }
      auto current_var_index = basic_var_index;
      std::ranges::sort(current_var_index);
      recorded_basic_var_index.insert(current_var_index);
    }

    pivot(pivot_row, pivot_column);
  }
}

// for use in two stage
std::optional<double> Simplex::getOptValue() const {
  if (solution_status == 0) {
    const double opt_value =
        obj_sense_changed ? tableau[0][var_num_total] : -tableau[0][var_num_total];
    return opt_value;
  }
  return std::nullopt;
}

void Simplex::pivot(const int pivot_row, const int pivot_column) {
  if (bool_record_tableau) {
    std::vector index = {pivot_row, pivot_column};
    recorded_pivot.emplace_back(index);
  }
  const double pivot_value = tableau[pivot_row][pivot_column];
  for (int j = 0; j < var_num_total + 1; j++) {
    tableau[pivot_row][j] /= pivot_value; // 除以 pivot_value
  }
  for (int i = 0; i < con_num + 1; i++) {
    if (i != pivot_row) {
      const double factor = tableau[i][pivot_column];
      for (int j = 0; j < var_num_total + 1; j++) {
        tableau[i][j] -= factor * tableau[pivot_row][j];
      }
    }
  }
  // 更新基变量：新进入变量替换旧基变量
  basic_var_index[pivot_row - 1] = pivot_column;
}

// 单纯形法实现
// Bland 规则确保单纯形法不会在退化解之间循环，最终会收敛到最优解或检测到无界解
// Bland 必须同时应用到换入和换出
int Simplex::findPivotColumn() const {
  int pivot_column = -1;
  double min_value = std::numeric_limits<double>::max();
  for (int j = 0; j < var_num_total; j++) {
    if (anti_cycle_rule == 0 or anti_cycle_rule == 2) { // none or lexi rule
      if (tableau[0][j] < -eps and tableau[0][j] < min_value) {
        pivot_column = j;
        min_value = tableau[0][j];
      }
    } else {                      // bland rule
      if (tableau[0][j] < -eps) { // 从索引 0 开始检查
        if (pivot_column == -1 || j < pivot_column) {
          pivot_column = j; // 选择索引最小的负检验数
        }
      }
    }
  }
  return pivot_column;
}

// Bland 规则确保单纯形法不会在退化解之间循环，最终会收敛到最优解或检测到无界解
int Simplex::findPivotRow(const int pivot_column) const {
  int pivot_row = -1;
  double min_ratio = std::numeric_limits<double>::max();

  for (int i = 1; i <= con_num; i++) {
    const double a_ij = tableau[i][pivot_column];
    if (a_ij <= eps)
      continue;

    const double ratio = tableau[i][var_num_total] / a_ij;

    // =========================
    // Dantzig（普通规则）
    // =========================
    if (anti_cycle_rule == 0) {
      if (ratio < min_ratio - eps) {
        min_ratio = ratio;
        pivot_row = i;
      }
    }

    // =========================
    // Bland
    // =========================
    else if (anti_cycle_rule == 1) {
      if (ratio < min_ratio - eps) {
        min_ratio = ratio;
        pivot_row = i;
      } else if (std::abs(ratio - min_ratio) <= eps) {
        // 比率相等时选最小索引
        if (pivot_row == -1 || basic_var_index[i - 1] < basic_var_index[pivot_row - 1]) {
          pivot_row = i;
        }
      }
    }

    // =========================
    // Lexicographic
    // =========================
    else if (anti_cycle_rule == 2) {
      // // 方式1：直接全行比较，其实两种方式等价
      // if (pivot_row == -1) {
      //   pivot_row = i;
      //   continue;
      // }
      // // lex compare
      // const double a_kj = tableau[pivot_row][pivot_column];
      // // RHS 必须第一个比较
      // double val_i = tableau[i][var_num_total] / a_ij;
      // double val_k = tableau[pivot_row][var_num_total] / a_kj;
      // if (val_i < val_k)
      //   pivot_row = i;
      // else
      //   continue;
      // for (int j = 0; j < var_num_total; j++) {
      //   val_i = tableau[i][j] / a_ij;
      //   val_k = tableau[pivot_row][j] / a_kj;
      //   if (std::abs(val_i - val_k) > eps) {
      //     if (val_i < val_k)
      //       pivot_row = i;
      //     break;
      //   }
      // }

      // 方式2：只对 ratio 相等的行采用lexi比较
      if (ratio < min_ratio - eps) {
        min_ratio = ratio;
        pivot_row = i;
      } else if (std::abs(ratio - min_ratio) <= eps) { // ratio 相等时再 lexi 比较
        // lex compare
        const double a_kj = tableau[pivot_row][pivot_column];
        // RHS 必须第一个比较
        double val_i = tableau[i][var_num_total] / a_ij;
        double val_k = tableau[pivot_row][var_num_total] / a_kj;
        if (val_i < val_k)
          pivot_row = i;
        for (int j = 0; j < var_num_total; j++) {
          val_i = tableau[i][j] / a_ij;
          val_k = tableau[pivot_row][j] / a_kj;
          if (std::abs(val_i - val_k) > eps) {
            if (val_i < val_k)
              pivot_row = i;
            break;
          }
        }
      }
    }
  }
  return pivot_row;
}

std::vector<double> Simplex::getOptSolution() const {
  if (solution_status == 0) {
    std::vector<int> indices(basic_var_index.size());
    std::iota(indices.begin(), indices.end(), 0); // fill the range sequentially with a start value
    // there is a lambda function below
    // class attribute vectors should use [this], not [&basic_vars]
    std::ranges::sort(indices, [this](const int i1, const int i2) {
      return basic_var_index[i1] < basic_var_index[i2];
    });
    std::vector<double> solution(var_num_start);
    for (int i = 0; i < basic_var_index.size(); i++) {
      const int var_index = basic_var_index[indices[i]];
      if (var_index > var_num_start - 1 + front_unsigned_num.back())
        continue;
      double value = tableau[indices[i] + 1][var_num_total];
      if (var_index > eps and
          front_unsigned_num[var_index] - front_unsigned_num[var_index - 1] > eps)
        value = -value;
      solution[var_index - front_unsigned_num[var_index]] = value;
      solution[var_index] = value;
    }
    return solution;
  }
  return {};
}

void Simplex::displaySolution() const {
  if (anti_cycle_rule == 0 and solution_status == 3)
    return;
  const double opt_value =
      obj_sense_changed ? tableau[0][var_num_total] : -tableau[0][var_num_total];
  std::cout << std::string(50, '*') << std::endl;
  std::cout << "The optimal value is: " << opt_value << std::endl;
  const auto solution = getOptSolution();
  for (size_t i = 0; i < solution.size(); i++)
    std::cout << "x_" << (i + 1) << " = " << solution[i] << std::endl;
}

void Simplex::checkInput() const {
  const size_t obj_coe_num = obj_coe.size();
  const size_t con_row_num = con_lhs.size();
  const size_t con_column_num = con_lhs[0].size();
  const size_t con_sense_num = constraint_sense.size();
  if (const size_t var_num = var_sign.size();
      !(obj_coe_num == var_num && var_num == con_column_num)) {
    std::cerr << "wrong input! Dimensions are not in consistent" << std::endl;
    exit(-1);
  }
  if (con_row_num != con_sense_num) {
    std::cerr << "wrong input!" << std::endl;
    exit(-1);
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

void Simplex::print() const {
  if (obj_sense == 0)
    std::cout << "min    ";
  else
    std::cout << "max    ";
  for (int i = 0; i < var_num_start; i++) {
    if (std::abs(-1 - obj_coe[i]) < eps)
      std::cout << "- ";
    else if (std::abs(1 - obj_coe[i]) > eps)
      std::cout << obj_coe[i];
    std::cout << "x_" << (i + 1) << " ";
    if (i != obj_coe.size() - 1 && obj_coe[i + 1] > -eps)
      std::cout << "+ ";
  }
  for (size_t i = 0; i < con_coe_slack.size(); i++) {
    if (std::abs(con_coe_slack[i]) > eps) {
      if (i != 0)
        std::cout << "+ ";
      std::cout << "0s_" << (i + 1);
    }
  }
  int artificial_count = 0;
  for (const int i : con_coe_artificial) {
    if (i != 0) {
      std::cout << " + ";
      std::cout << "0a_" << (artificial_count + 1) << "";
      artificial_count++;
    }
  }
  std::cout << std::endl;
  std::cout << "s.t." << std::endl;
  for (int j = 0; j < con_num; j++) {
    for (size_t i = 0; i < var_num_start; i++) {
      if (std::abs(-1 - con_lhs[j][i]) < eps)
        std::cout << "-";
      else if (std::abs(1 - con_lhs[j][i]) > eps && con_lhs[j][i] > -eps)
        std::cout << con_lhs[j][i];
      else if (std::abs(1 - con_lhs[j][i]) > eps && con_lhs[j][i] < -eps)
        std::cout << "-" << -con_lhs[j][i];
      std::cout << "x_" << (i + 1);
      if (i != var_num_start - 1 && con_lhs[j][i + 1] > -eps)
        std::cout << " + ";
      if (i != var_num_start - 1 && con_lhs[j][i + 1] < -eps)
        std::cout << " ";
    }
    if (var_num_slack > 1e-1) {
      int slack_count = 0;
      for (size_t k = 0; k <= j; k++) {
        if (std::abs(con_coe_slack[k]) > eps)
          slack_count++;
      }
      if (std::abs(con_coe_slack[j]) > eps) {
        for (int m = 0; m < slack_count - 1; m++)
          std::cout << " + 0s_" << (m + 1);
        if (con_coe_slack[j] == 1)
          std::cout << " + s_" << (slack_count);
        else if (std::abs(-1 - con_coe_slack[j]) < eps)
          std::cout << " -s_" << (slack_count);
        for (int m = slack_count + 1; m <= var_num_slack; m++)
          std::cout << " + 0s_" << (m);
      } else {
        for (int m = 0; m < var_num_slack; m++)
          std::cout << " + 0s_" << (m + 1);
      }
    }
    if (var_num_artificial > 1e-1) {
      artificial_count = 0;
      for (size_t k = 0; k <= j; k++) {
        if (std::abs(con_coe_artificial[k]) > eps)
          artificial_count++;
      }
      if (con_coe_artificial[j] == 1) {
        for (int m = 0; m < artificial_count - 1; m++)
          std::cout << " + 0a_" << (m + 1);
        std::cout << " + a_" << (artificial_count);
        for (int m = artificial_count + 1; m <= var_num_artificial; m++)
          std::cout << " + 0a_" << (m);
      } else {
        for (int m = 0; m < var_num_artificial; m++)
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
  for (int i = 0; i < var_num_start; i++) {
    if (var_sign[i] != 2 && i != 0)
      std::cout << ", ";
    if (var_sign[i] == 0)
      std::cout << "x_" << (i + 1) << " >= 0";
    if (var_sign[i] == 1)
      std::cout << "x_" << (i + 1) << " <= 0";
  }
  for (size_t i = 0; i < con_coe_slack.size(); i++) {
    if (std::abs(con_coe_slack[i]) > eps) {
      std::cout << ", ";
      std::cout << "s_" << (i + 1) << " >= 0";
    }
  }
  artificial_count = 0;
  for (const int i : con_coe_artificial) {
    if (i != 0) {
      std::cout << ", ";
      std::cout << "a_" << (artificial_count + 1) << " >= 0";
      artificial_count++;
    }
  }
  std::cout << std::endl;
}

void Simplex::printAllTableau() const {
  std::cout << std::string(50, '*') << std::endl;
  for (size_t i = 0; i < recorded_tableau.size(); i++) {
    std::cout << "iteration step " << i + 1 << std::endl;
    for (const auto &row : recorded_tableau[i]) {
      for (const auto column : row)
        std::cout << std::fixed << std::setprecision(2) << column << "   ";
      std::cout << std::endl;
    }
  }
}

void Simplex::setAntiCycle(const int rule) { anti_cycle_rule = rule; }

// 检查 cycling
bool detectCycling(std::vector<int> current_basis, std::set<std::vector<int>> basis_history) {
  // 1. 必须排序！因为基集合的顺序不影响它代表的基
  std::ranges::sort(current_basis);

  // 2. 尝试插入到历史记录中
  // std::set::insert 返回一个 pair，其第二个成员 bool 表示是否插入成功
  auto [fst, snd] = basis_history.insert(current_basis);

  if (!snd) {
    return true;
  }

  return false;
}

// int main() {
//   // 初始化单纯形表
//   // 标准化的单纯性表，目标函数为 max
//   // 目标函数: max z = 2x1 + 3x2 转换为 z -2x1 - 3x2
//   // 约束: 2x1 + x2 + s1 = 4
//   //       x1 + 2x2 + s2 = 5
//
//   // constexpr int obj_sense = 1;
//   // const std::vector obj_coe = {2.0, 3.0};
//   // const std::vector<std::vector<double>> con_lhs = {{2, 1}, {1, 2}};
//   // const std::vector con_rhs = {4.0, 5.0};
//   // const std::vector constraint_sense = {0, 0};
//   // const std::vector var_sign = {0, 0};
//
//   // constexpr int obj_sense = 1; // 0: min, 1: max, basic var already
//   // const std::vector obj_coe = {3.0, 5.0, 0.0, 0.0, 0.0};
//   // const std::vector<std::vector<double>> con_lhs = {
//   //     {1.0, 0.0, 1.0, 0, 0},
//   //     {0, 2.0, 0, 1.0, 0.0},
//   //     {3, 2.0, 0, 0.0, 1.0},
//   // };
//   // const std::vector con_rhs = {4.0, 12.0, 18.0};
//   // const std::vector constraint_sense = {2, 2, 2}; // 0:<=, 1: >=, 2: =
//   // const std::vector var_sign = {0, 0, 0, 0, 0};   // 0: >=, 1: <=, 2: unsigned
//
//   // constexpr int obj_sense = 0;
//   // const std::vector obj_coe = {50.0, 20.0, 30.0, 80.0}; // two-stage
//   // const std::vector<std::vector<double>> con_lhs = {
//   //     {400, 200, 100, 500}, {3, 2, 0, 0}, {2, 2, 4, 4}, {2, 4, 1, 5}};
//   // const std::vector con_rhs = {500.0, 6.0, 10.0, 8.0};
//   // const std::vector constraint_sense = {1, 1, 1, 1};
//   // const std::vector var_sign = {0, 0, 0, 0};
//
//   // constexpr int obj_sense = 1; // two-stage
//   // const std::vector obj_coe = {1.0, 5.0, 3.0};
//   // const std::vector<std::vector<double>> con_lhs = {
//   //     {1, 2, 1},
//   //     {2, -1, 0},
//   // };
//   // const std::vector con_rhs = {3.0, 4.0};
//   // const std::vector constraint_sense = {2, 2};
//   // const std::vector var_sign = {0, 0, 0};
//
//   // constexpr int obj_sense = 1; // cycling
//   // const std::vector<double> obj_coe = {0.75, -20, 0.5, -6};
//   // const std::vector<std::vector<double>> con_lhs = {
//   //     {0.25, -8, -1, 9}, {0.5, -12, -0.5, 3}, {0, 0, 1, 0}};
//   // const std::vector con_rhs = {0.0, 0.0, 1.0};
//   // const std::vector constraint_sense = {0, 0, 0};
//   // const std::vector var_sign = {0, 0, 0, 0};
//
//   constexpr int obj_sense = 1; // unsigned
//   const std::vector<double> obj_coe = {-4, 10, -5};
//   const std::vector<std::vector<double>> con_lhs = {{-1, 2, -1}, {1, 3, -1}, {0, -1, 2}};
//   const std::vector con_rhs = {-2.0, 14.0, 2.0};
//   const std::vector constraint_sense = {2, 0, 1};
//   const std::vector var_sign = {0, 0, 2};
//
//   // constexpr int obj_sense = 0; // two-stage with column and row elimination
//   // const std::vector obj_coe = {1.0, 1.0, 1.0, 0.0};
//   // const std::vector<std::vector<double>> con_lhs = {
//   //     {1, 2, 3, 0}, {-1, 2, 6, 0}, {0, 4, 9, 0}, {0, 0, 3, 1}};
//   // const std::vector con_rhs = {3.0, 2.0, 5.0, 1.0};
//   // const std::vector constraint_sense = {2, 2, 2, 2};
//   // const std::vector var_sign = {0, 0, 0, 0};
//
//   // constexpr int obj_sense = 1;
//   // const std::vector obj_coe = {-3.0, 0.0, 1.0};
//   // const std::vector<std::vector<double>> con_lhs = {{1, 1, 1}, {-2, 1, -1}, {0, 3, 1}};
//   // const std::vector con_rhs = {4.0, 1.0, 9.0};
//   // const std::vector constraint_sense = {0, 1, 2};
//   // const std::vector var_sign = {0, 0, 0};
//
//   // constexpr int obj_sense = 0;
//   // const std::vector obj_coe = {2.0, 3.0, 1.0};
//   // const std::vector<std::vector<double>> con_lhs = {{1, 4, 2}, {3, 2, 0}};
//   // const std::vector con_rhs = {8.0, 6.0};
//   // const std::vector constraint_sense = {1, 1};
//   // const std::vector var_sign = {0, 0, 0};
//
//   auto model =
//       Simplex(obj_sense, obj_coe, con_lhs, con_rhs, constraint_sense, var_sign, anti_cycle_rule);
//   model.checkInput();
//   std::cout << "original model is:" << std::endl;
//   model.print();
//
//   model.standardize();
//   std::cout << std::string(50, '*') << std::endl;
//   std::cout << "the standardized model is:" << std::endl;
//   model.print();
//   const auto start_time = std::chrono::high_resolution_clock::now();
//   model.solve();
//   const auto end_time = std::chrono::high_resolution_clock::now();
//   model.displaySolution();
//   if (bool_record_tableau)
//     model.printAllTableau();
//   const std::chrono::duration<double> diff = end_time - start_time;
//   std::cout << std::fixed << std::setprecision(6) << "cpu time is: " << diff.count() << "seconds"
//             << std::endl;
//
//   // // 初始化单纯形表
//   // //  目标函数: max z = 2x1 + 3x2 转换为 -2x1 - 3x2 + M*a1 + M*a2 + z =
//   // 0
//   // //  约束: x1+x2 >= 2, i.e.,  x1 + x2 - s1 + a1 = 2
//   // //       2x1+x2 = 4, i.e., 2x1 + x2 + a2 = 4
//   // const std::vector<std::vector<double>> tableau2 = {
//   //     {-2, -3, 1, -M, -M, 0}, // 目标函数 (x1, x2, s1, a1, a2, z)
//   //     {1, 1, -1, 1, 0, 2},    // 约束1
//   //     {2, 1, 0, 0, 1, 4}      // 约束2
//   // };
//   // Simplex simplex2(tableau2);
//   // simplex2.solve();
//   //
//   return 0;
//
//   // const std::vector<std::vector<double>> A = {{1, 0, 2}, {0, 3, 0}, {4,
//   // 0, 5}};
//   //
//   // const CSC csc = denseToCSC(A);
//   // printCSC(csc);
//   // for (int i = 0; i < A.size(); i++) {
//   //   std::cout << std::endl;
//   //   for (int j = 0; j < A[0].size(); j++) {
//   //     std::cout << getValue(csc, i, j) << " ";
//   //   }
//   // }
// }