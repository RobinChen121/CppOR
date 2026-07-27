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
    first = false;
  }

  if (first)
    std::cout << "0";
}

void Model::printObjective() const {
  std::cout << (obj_sense == ObjSense::Minimize ? "Minimize\n" : "Maximize\n");
  std::cout << " obj: ";
  bool first = true;

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
    first = false;
  }

  if (first)
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
    } else {
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

void Model::print() const {
  std::cout << "========================================\n";

  printObjective();
  printConstraints();
  printBounds();
  printIntegers();
  printBinaries();

  std::cout << "End\n";
  std::cout << "========================================\n";
}
