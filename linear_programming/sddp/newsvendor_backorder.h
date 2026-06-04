/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 04/06/2026, 19:03
 * Description:
 *
 */

#ifndef CHEN_SOLVER_JS_NEWSVENDOR_BACKORDER_GUROBI_H
#define CHEN_SOLVER_JS_NEWSVENDOR_BACKORDER_GUROBI_H

#include <array>
#include <vector>

class Newsvendor {
  double ini_I = 0.0;
  int T = 10;
  std::vector<double> unit_vari_costs = std::vector(T, 1.0);
  std::vector<double> unit_holding_costs = std::vector(T, 2.0);
  std::vector<double> unit_backorder_costs = std::vector(T, 10.0);
  double mean_demand = 20.0;
  std::vector<double> mean_demands = std::vector<double>(T, mean_demand);

  // SDDP settings
  int sample_num = 10; // 10;
  int forward_num = 30;
  int iter_num = 50;
  double theta_lb = 0;

public:
  Newsvendor() = default;
  [[nodiscard]] std::array<double, 2> solve() const;
};

#endif // CHEN_SOLVER_JS_NEWSVENDOR_BACKORDER_GUROBI_H
