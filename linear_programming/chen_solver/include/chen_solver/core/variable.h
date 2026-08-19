/*
 * Created by Zhen Chen on 2026/8/19.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef CHEN_SOLVER_CORE_VARIABLE_H
#define CHEN_SOLVER_CORE_VARIABLE_H

#include "linear_programming/chen_solver/include/chen_solver/config.h"
#include "string"

struct Variable {
  VarId id;
  std::string name;
  double lb = 0.0;
  double ub = INF;
  VarType var_type = VarType::Continuous;
};

#endif // CHEN_SOLVER_CORE_VARIABLE_H
