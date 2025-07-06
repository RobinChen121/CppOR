//
// Created by Administrator on 2025/7/6.
//

#ifndef PIECEWISE_H
#define PIECEWISE_H

#include <vector>

class PiecewiseWorkforce {
  int initial_workers{};
  double fix_hire_cost{};
  double unit_vari_cost = {};
  double salary{};
  double unit_penalty{};

  std::vector<double> turnover_rate{};
  size_t T = turnover_rate.size();
  std::vector<int> min_workers = std::vector<int>(T);

public:
  PiecewiseWorkforce(const int initial_workers, const double fix_hire_cost,
                     const double unit_vari_cost, const double salary, const double unit_penalty,
                     const std::vector<double> &turnover_rate, const std::vector<int> &min_workers)
      : initial_workers(initial_workers), fix_hire_cost(fix_hire_cost),
        unit_vari_cost(unit_vari_cost), salary(salary), unit_penalty(unit_penalty),
        turnover_rate(turnover_rate), min_workers(min_workers) {};

  static double loss_function(int y, int min_workers, double turnover_rate);
};


#endif // PIECEWISE_H
