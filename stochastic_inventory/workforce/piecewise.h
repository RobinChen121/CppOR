//
// Created by Administrator on 2025/7/6.
//

#ifndef PIECEWISE_H
#define PIECEWISE_H

#include <array>
#include <map>
#include <vector>

class PiecewiseWorkforce {
  using PiecewiseResult = std::vector<std::vector<double>>;
  using PiecewiseKey = std::pair<int, int>;

  int initial_workers{};
  double fix_hire_cost{};
  double unit_vari_cost = {};
  double salary{};
  double unit_penalty{};

  std::vector<double> turnover_rates{};
  size_t T = turnover_rates.size();
  std::vector<int> min_workers = std::vector<int>(T);
  // 用来存缓存的变量就必须加上 mutable 关键字，从而允许在 const 函数中被修改
  mutable std::map<PiecewiseKey, PiecewiseResult> piecewise_cache_;
  int segment_num_ = 1;

public:
  PiecewiseWorkforce(const int initial_workers, const double fix_hire_cost,
                     const double unit_vari_cost, const double salary, const double unit_penalty,
                     const std::vector<double> &turnover_rates, const std::vector<int> &min_workers,
                     const int segment_num)
      : initial_workers(initial_workers), fix_hire_cost(fix_hire_cost),
        unit_vari_cost(unit_vari_cost), salary(salary), unit_penalty(unit_penalty),
        turnover_rates(turnover_rates), min_workers(min_workers), segment_num_(segment_num) {};

  static PiecewiseResult piecewise(int segment_num, int min_worker, double p);
  std::pair<double, double> pieceApproximateCallback(int segment_num) const;
  void preparePiecewiseCache(int segment_num) const;
  void preparePiecewiseCache() const;
  [[nodiscard]] const PiecewiseResult &getPiecewiseResult(int segment_num, int t, int j) const;

  [[nodiscard]] std::pair<double, double> pieceApproximate(int segment_num) const;
  [[nodiscard]] double computeLineGap(const std::vector<int> &z, const std::vector<double> &y,
                                      const std::vector<double> &u) const;
  [[nodiscard]] std::vector<std::array<int, 2>> get_sS(int segment_num) const;
  [[nodiscard]] int find_s(int segment_num, int S_value, double GS, int tt) const;
};

#endif // PIECEWISE_H
