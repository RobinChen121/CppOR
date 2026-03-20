/*
 * Created by Zhen Chen on 2026/3/20.
 * Email: chen.zhen5526@gmail.com
 * Description: This simplex algorithm aims to process sparse matrix.
 *
 *
 */

#ifndef WORKFORCE_SIMPLEX_H
#define WORKFORCE_SIMPLEX_H

#include <vector>

constexpr double epsilon = 1e-6;

class Simplex {
  std::vector<double> obj_coe;
  int obj_sense{}; // 0:min, 1: max
  std::vector<std::vector<double>> con_lhs;
  std::vector<double> con_rhs;
  std::vector<int> constraint_sense; // 0:<=, 1: >=, 2: =
  std::vector<int> var_sign;         // 0: >=, 1: <=, 2: unsigned
  int solution_status = {3};         // 0 optimal, 1 unbounded, 2 infeasible, 3 unsolved
};

#endif // WORKFORCE_SIMPLEX_H
