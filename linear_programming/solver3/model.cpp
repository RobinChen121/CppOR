/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 25/07/2026, 22:07
 * Description:
 *
 */

#include "model.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

void Model::addVariable(const std::string &name, const double lb, const double ub,
                        const VarType type) {
  const std::string actual_name = name.empty() ? "x" + std::to_string(next_var_id) : name;

  if (name_to_varIndex.contains(actual_name))
    throw std::runtime_error("Duplicate variable name");

  const auto col = static_cast<ChenInt>(variables.size());
  variables.push_back(Variable{next_var_id++, actual_name, lb, ub, type});

  name_to_varIndex[variables.back().name] = col;
  status = ModelStatus::Modified;
};

void Model::addConstraint(const std::string &name, const std::vector<LinearTerm> &terms,
                          const double lb, const double ub) {
  const std::string actual_name = name.empty() ? "c" + std::to_string(next_con_id) : name;

  if (name_to_conIndex.contains(actual_name)) {
    throw std::runtime_error("Constraint already exists: " + actual_name);
  }

  Constraint con;
  con.id = next_con_id++;
  con.name = actual_name;
  con.lb = lb;
  con.ub = ub;

  // 合并重复列
  std::map<ChenInt, double> coeffs;
  for (const auto &[col, coef] : terms) {
    coeffs[col] += coef;
  }

  // 然后把 coeffs 传递到 con 里
  for (const auto &[col, coef] : coeffs) {
    if (std::abs(coef) > 1e-12) {
      con.lhs.push_back({col, coef});
    }
  }

  constraints.push_back(std::move(con));
  name_to_conIndex[name] = static_cast<ChenInt>(constraints.size() - 1);
  status = ModelStatus::Modified;
}

[[nodiscard]] bool Model::valid() const {
  if (std::ranges::any_of(variables, [](const auto &c) { return c.lb > c.ub; }))
    return false;
  if (std::ranges::all_of(constraints, [](const auto &c) { return c.lb < c.ub; }))
    return false;
  for (auto const &con : constraints) {
    for (const auto &[col, coef] : con.lhs) {
      if (col < 0 || col >= variables.size())
        return false;
    }
  }
  return true;
}

// 求解器中使用 nameToVarIndex
ChenInt Model::nameToVarIndex(const std::string &name) const {
  const auto it = name_to_varIndex.find(name);
  if (it == name_to_varIndex.end())
    throw std::runtime_error("Unknown variable: " + name);
  return it->second;
}

// LP/MPS Reader 中统一使用：findOrCreateVariable
ChenInt Model::findOrCreateVariable(const std::string &name) {
  const auto it = name_to_varIndex.find(name);
  if (it != name_to_varIndex.end())
    return it->second;

  addVariable(name);
  return static_cast<ChenInt>(variables.size() - 1);
}

void Model::printLinearExpression(const std::vector<LinearTerm> &terms) const {
  bool first = true;

  size_t counter = 0;
  for (const auto &[col, coef] : terms) {
    if (std::abs(coef) < 1e-12)
      continue;
    if (!first) {
      std::cout << (coef >= 0 ? " + " : " - ");
    } else if (coef < 0) {
      std::cout << "-";
    }

    if (const double abs_coef = std::abs(coef); std::abs(abs_coef - 1.0) > 1e-12)
      std::cout << abs_coef << " ";
    std::cout << variables[col].name;
    counter++;
    if (counter % 3 == 0)
      std::cout << "\n      ";
    first = false;
  }

  if (first)
    std::cout << "0";
}

void Model::printObjective() const {
  std::cout << (obj_sense == ObjSense::Minimize ? "Minimize\n" : "Maximize\n");
  std::cout << " obj: ";
  bool first = true;

  size_t counter = 0;
  for (auto const &[col, coef] : objective_coef) {
    if (std::abs(coef) < 1e-12)
      continue;

    if (!first) {
      std::cout << (coef >= 0 ? " + " : " - ");
    } else if (coef < 0) {
      std::cout << "-";
    }

    if (const double abs_coef = std::abs(coef); std::abs(abs_coef - 1.0) > 1e-12)
      std::cout << abs_coef << " ";
    std::cout << variables[col].name;
    counter++;
    if (counter % 3 == 0)
      std::cout << "\n      ";
    first = false;
  }

  if (first) // 目标函数全部为0
    std::cout << "0";
  std::cout << "\n\n";
}

void Model::printConstraints() const {
  std::cout << "Subject To\n";
  for (auto &con : constraints) {
    std::cout << " " << (con.name.empty() ? "c" : con.name) << ": ";
    printLinearExpression(con.lhs);

    if (std::abs(con.lb - con.ub) < 1e-12) {
      std::cout << " = " << con.ub;
    } else if (con.lb <= -INF / 2) {
      std::cout << " <= " << con.ub;
    } else if (con.ub >= INF / 2) {
      std::cout << " >= " << con.lb;
    } else { // 处理约束条件在可能的 ranges 的情况
      std::cout << " in [" << con.lb << ", " << con.ub << "]";
    }

    std::cout << "\n";
  }

  std::cout << "\n";
}

void Model::printBounds() const {
  std::cout << "Bounds\n";
  for (const auto &var : variables) {
    if (var.var_type == VarType::Binary) {
      continue;
    }
    if (var.lb <= -INF / 2 && var.ub >= INF / 2) {
      std::cout << " " << var.name << " free\n";
    } else if (std::abs(var.lb - var.ub) < 1e-12) {
      std::cout << " " << var.name << " = " << var.lb << "\n";
    } else if (var.lb > -INF / 2 && var.ub < INF / 2) {
      std::cout << " " << var.lb << " <= " << var.name << " <= " << var.ub << "\n";
    } else if (var.lb > -INF / 2) {
      std::cout << " " << var.name << " >= " << var.lb << "\n";
    } else if (var.ub < INF / 2) {
      std::cout << " " << var.name << " <= " << var.ub << "\n";
    }
  }

  std::cout << "\n";
}

void Model::printIntegers() const {
  bool has_integer = false;
  for (const auto &var : variables)
    if (var.var_type == VarType::Integer)
      has_integer = true;

  if (!has_integer)
    return;

  std::cout << "Generals\n";
  for (const auto &var : variables)
    if (var.var_type == VarType::Integer)
      std::cout << " " << var.name << "\n";

  std::cout << "\n";
}

void Model::printBinaries() const {
  bool has_binary = false;
  for (const auto &var : variables)
    if (var.var_type == VarType::Binary)
      has_binary = true;

  if (!has_binary)
    return;

  std::cout << "Binaries\n";
  for (const auto &var : variables)
    if (var.var_type == VarType::Binary)
      std::cout << " " << var.name << "\n";

  std::cout << "\n";
}

SolverModel compile(const Model &model) {
  SolverModel solver_model;

  // 1. 基础元数据复制与维度初始化
  solver_model.obj_sense = model.obj_sense;
  solver_model.obj_offset = 0.0; // 若 Model 增加 offset 字段可在此处赋值

  const ChenInt num_cols = static_cast<ChenInt>(model.variables.size());
  const ChenInt num_rows = static_cast<ChenInt>(model.constraints.size());

  solver_model.num_col = num_cols;
  solver_model.num_row = num_rows;

  // 2. 提取变量界限、类型和 ID，以及密集的目标函数系数
  solver_model.col_lb.resize(num_cols);
  solver_model.col_ub.resize(num_cols);
  solver_model.var_type.resize(num_cols);
  solver_model.col_ids.resize(num_cols);

  for (ChenInt j = 0; j < num_cols; ++j) {
    const auto &var = model.variables[j];
    solver_model.col_lb[j] = var.lb;
    solver_model.col_ub[j] = var.ub;
    solver_model.var_type[j] = var.var_type;
    solver_model.col_ids[j] = var.id;
  }

  // 稀疏目标函数转化为稠密向量 (大小等于变量个数)
  solver_model.objective_coef.assign(num_cols, 0.0);
  for (const auto &[col_idx, coef] : model.objective_coef) {
    if (col_idx >= 0 && col_idx < num_cols) {
      solver_model.objective_coef[col_idx] = coef;
    }
  }

  // 3. 提取约束上下界和 ID
  solver_model.row_lb.resize(num_rows);
  solver_model.row_ub.resize(num_rows);
  solver_model.row_ids.resize(num_rows);

  for (ChenInt i = 0; i < num_rows; ++i) {
    const auto &con = model.constraints[i];
    solver_model.row_lb[i] = con.lb;
    solver_model.row_ub[i] = con.ub;
    solver_model.row_ids[i] = con.id;
  }

  // 4. 构建 CSC 矩阵 (核心：行格式 -> 稀疏列格式)

  // 步骤 4.1：统计矩阵每一列（每个变量）的非零元数量
  std::vector<ChenInt> col_counts(num_cols, 0);
  ChenInt total_nnz = 0;

  for (const auto &con : model.constraints) {
    for (const auto &[col, coef] : con.lhs) {
      if (col >= 0 && col < num_cols) {
        col_counts[col]++;
        total_nnz++;
      }
    }
  }

  // 初始化求解器的 CSC 结构，一次性预分配内存
  solver_model.A_matrix = CSC(total_nnz, num_cols);
  CSC &csc = solver_model.A_matrix;

  // 步骤 4.2：构建 col_ptr，并准备一个游标数组用于桶排序 (Bucket Sort)
  csc.col_ptr[0] = 0;
  for (ChenInt j = 0; j < num_cols; ++j) {
    csc.col_ptr[j + 1] = csc.col_ptr[j] + col_counts[j];
  }

  // current_col_offset 记录当前列下一个非零元应该填入 csc.values 的位置
  std::vector<ChenInt> current_col_offset = csc.col_ptr;

  // 步骤 4.3：遍历所有约束，将非零元填入对应的列中
  for (ChenInt row_idx = 0; row_idx < num_rows; ++row_idx) {
    const auto &con = model.constraints[row_idx];
    for (const auto &term : con.lhs) {
      const ChenInt col_idx = term.col;
      if (col_idx >= 0 && col_idx < num_cols) {
        // 获取当前列在 CSC 数组中的写入位置
        const ChenInt dest_idx = current_col_offset[col_idx]++;

        csc.row_indices[dest_idx] = row_idx;
        csc.values[dest_idx] = term.coef;
      }
    }
  }

  return solver_model;
}

void Model::print() const {
  std::cout << "========================================\n";

  if (!model_name.empty())
    std::cout << "Model Name: " << model_name << "\n\n";

  printObjective();
  printConstraints();
  printBounds();
  printIntegers();
  printBinaries();

  std::cout << "End\n";
  std::cout << "========================================\n";
}
