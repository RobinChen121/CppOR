/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 24/06/2026, 22:59
 * Description:
 *
 */

#ifndef CHEN_SOLVER_JS_READ_FILE_H
#define CHEN_SOLVER_JS_READ_FILE_H

#include "config.h"

#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

struct ParsedModel {
  int obj_sense = 0;
  std::vector<std::string> var_names;
  std::unordered_map<std::string, int> var_index;
  std::unordered_map<int, double> objective;
  std::vector<std::unordered_map<int, double>> lhs;
  std::vector<double> rhs;
  std::vector<int> constraint_sense; // <=: 0, =: 1, >=: 2
  std::vector<int> var_type;         // 0: continuous, 1: integer, 2: binary
  std::vector<double> lower_bound;
  std::vector<double> upper_bound;
  std::vector<bool> free_var;

  int ensureVar(const std::string &name) {
    if (const auto it = var_index.find(name); it != var_index.end())
      return it->second;
    const int index = static_cast<int>(var_names.size());
    var_index[name] = index;
    var_names.push_back(name);
    lower_bound.push_back(0.0);
    upper_bound.push_back(INF);
    free_var.push_back(false);
    var_type.push_back(0);
    return index;
  }

  void addConstraint(const std::unordered_map<int, double> &row, const int sense,
                     const double value) {
    lhs.push_back(row);
    constraint_sense.push_back(sense);
    rhs.push_back(value);
  }

  void print();
};

ParsedModel readLP(const std::string &path);
ParsedModel readMPS(const std::string &path);

ParsedModel read(const std::string &path);

#endif // CHEN_SOLVER_JS_READ_FILE_H
