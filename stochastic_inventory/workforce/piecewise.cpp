//
// Created by Administrator on 2025/7/6.
//

#include "piecewise.h"
#include <boost/math/distributions/binomial.hpp> // 二项分布头文件; random 库有分布但没有pdf函数，是生成随机数的


double PiecewiseWorkforce::loss_function(const int y, const int min_workers, const double
turnover_rate) {
  const boost::math::binomial_distribution<double> dist(y, turnover_rate);
  const int i_min = std::max(y - min_workers, 0);
  double value = 0;
  for (int i = i_min; i <= y; i++) {
    value += pdf(dist, i) * (i + min_workers - y);
  }
  return value;
}

