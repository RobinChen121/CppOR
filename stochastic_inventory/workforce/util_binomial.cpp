/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 07/08/2025, 20:51
 * Description: 
 * 
 */

#include "util_binomial.h"
#include <boost/math/distributions/binomial.hpp> // for binomial distribution cdf and pdf, random library only for generating random numbers

double loss_function_expect(const int y, const int min_worker,
                                         const double turnover_rate) {
  const boost::math::binomial_distribution<double> dist(y, turnover_rate);
  const int i_min = std::max(y - min_worker, 0);
  double value = 0;
  for (int i = i_min; i <= y; i++) {
    value += pdf(dist, i) * (min_worker - y + i);
  }
  return value;
}

double Fy_y_minus_w(const int y, const int min_worker, const double turnover_rate) {
  const boost::math::binomial_distribution<double> dist(y, turnover_rate);
  if (y - min_worker < 0) {
    return 0;
  }
  return cdf(dist, y - min_worker);
}