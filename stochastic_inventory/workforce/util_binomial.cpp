/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 07/08/2025, 20:51
 * Description:
 *
 */

#include "util_binomial.h"
#include <boost/math/distributions/binomial.hpp> // for binomial distribution cdf and pdf, random library only for generating random numbers

// Binomial(n, p) PMF: P(X = k)
double binomialPDF(const int n, const double p, const int k) {
  if (k < 0 || k > n)
    return 0.0;
  if (p < 0.0 || p > 1.0)
    return std::numeric_limits<double>::quiet_NaN();

  if (p == 0.0)
    return (k == 0) ? 1.0 : 0.0;
  if (p == 1.0)
    return (k == n) ? 1.0 : 0.0;

  // lgamma(k+1) gives log(k!)
  // 通过计算log，避免较大的阶乘溢出
  const double log_prob = std::lgamma(n + 1.0) - std::lgamma(k + 1.0) - std::lgamma(n - k + 1.0) +
                          k * std::log(p) + (n - k) * std::log(1.0 - p);

  return std::exp(log_prob);
}

double binomialCDF(const int n, const double p, const int k) {
  if (k < 0)
    return 0.0;

  if (k >= n)
    return 1.0;
  if (p <= 0.0)
    return 1.0;
  if (p >= 1.0)
    return (k >= n) ? 1.0 : 0.0;

  double prob = std::pow(1.0 - p, n); // P(X=0)
  double cdf = prob;
  for (int i = 0; i < k; ++i) {
    // P(X=i+1) from P(X=i)，利用相邻元素的关系递推求解
    prob *= (static_cast<double>(n - i) / (i + 1)) * (p / (1.0 - p));
    cdf += prob;
  }

  return cdf;
}

double lossFunctionExpect(const int y, const int min_worker, const double turnover_rate) {
  const int i_min = std::max(y - min_worker, 0);
  double value = 0;
  for (int i = i_min; i <= y; i++) {
    value += binomialPDF(y, turnover_rate, i) * (min_worker - y + i);
  }
  return value;
}

double Fy_y_minus_w(const int y, const int min_worker, const double turnover_rate) {
  if (y - min_worker < 0) {
    return 0;
  }
  return binomialCDF(y, turnover_rate, y - min_worker);
}