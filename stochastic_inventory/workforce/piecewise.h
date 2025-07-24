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

  std::vector<double> turnover_rates{};
  size_t T = turnover_rates.size();
  std::vector<int> min_workers = std::vector<int>(T);

public:
  PiecewiseWorkforce(const int initial_workers, const double fix_hire_cost,
                     const double unit_vari_cost, const double salary, const double unit_penalty,
                     const std::vector<double> &turnover_rates, const std::vector<int> &min_workers)
      : initial_workers(initial_workers), fix_hire_cost(fix_hire_cost),
        unit_vari_cost(unit_vari_cost), salary(salary), unit_penalty(unit_penalty),
        turnover_rates(turnover_rates), min_workers(min_workers) {};

  static double loss_function(int y, int min_workers, double turnover_rate);
  static double Fy(int y, int min_workers, double turnover_rate);
  static std::vector<std::vector<double>> piecewise(int segment_num, int min_workers, double p);

  [[nodiscard]] double piece_approximate(int segment_num) const;
  [[nodiscard]] std::vector<std::array<double, 2>> get_sS(int segment) const;
  [[nodiscard]] double find_s(int segment_num, double S_value, double GS, int tt) const;
};

#endif // PIECEWISE_H
