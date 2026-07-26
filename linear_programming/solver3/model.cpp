/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 25/07/2026, 22:07
 * Description:
 *
 */

#include "model.h"

#include <iostream>
#include <stdexcept>

void Model::addVariable(std::string &name, const double lb, const double ub, const VarType type) {
  if (name.empty())
    name = "x" + std::to_string(next_var_id);
  if (name_to_varIndex.contains(name))
    throw std::runtime_error("Duplicate variable name");

  const auto col = static_cast<ChenInt>(variables.size());
  variables.push_back(Variable{next_var_id++, name, lb, ub, type});

  name_to_varIndex[variables.back().name] = col;
};

void Model::addConstraint(std::string &name, const std::vector<LinearTerm> &terms, const double lb,
                          const double ub) {
  if (name.empty())
    name = "c" + std::to_string(next_con_id);

  if (name_to_conIndex.contains(name)) {
    throw std::runtime_error("Constraint already exists: " + name);
  }

  Constraint con;
  con.id = next_con_id++;
  con.name = name;
  con.lb = lb;
  con.ub = ub;

  // 合并重复列
  std::unordered_map<ChenInt, double> coeffs;
  for (const auto &[col, coef] : terms) {
    if (col < 0 || col >= static_cast<ChenInt>(variables.size())) {
      throw std::runtime_error("Invalid variable index in constraint " + name);
    }
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
}

[[nodiscard]] bool Model::valid() const {
  for (auto const &v : variables)
    if (v.lb > v.ub)
      return false;

  for (auto const &c : constraints)
    if (c.lb > c.ub)
      return false;
  return variables.size() == objective_coef.size();
}

ChenInt Model::nameToVarIndex(std::string &name) {
  if (const auto it = name_to_varIndex.find(name); it != name_to_varIndex.end())
    return it->second;
  const int index = static_cast<int>(name_to_varIndex.size());
  name_to_varIndex[name] = index;
  addVariable(name, 0.0, INF, VarType::Continuous);
  // free_var.push_back(false);
  return index;
}

void Model::printLinearExpression(std::vector<LinearTerm> &terms) const {
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

void Model::printObjective() {
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

void Model::printConstraints() {
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
    if (var.var_type == VarType::Binary)
      continue;

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

void Model::print() {
  std::cout << "========================================\n";

  printObjective();
  printConstraints();
  printBounds();
  printIntegers();
  printBinaries();

  std::cout << "End\n";
  std::cout << "========================================\n";
}
