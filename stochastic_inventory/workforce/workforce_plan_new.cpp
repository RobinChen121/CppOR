/*
 * Created by Zhen Chen on 2026/8/31.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "workforce_plan_new.h"

#include <cmath>
#include <iostream>
#include <limits>

// Binomial PMF: P(X = k)
double binomialPdf(const int n, const int k, const double p) {
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

PMFData getPMFBinomial(const int max_staff, const std::vector<double> &ps) {
  const size_t T = ps.size();
  PMFData result;

  result.offset.resize(max_staff + 1);
  for (int i = 0; i <= max_staff; ++i) {
    result.offset[i] = static_cast<size_t>(i) * (i + 1) / 2;
  }

  result.per_t = static_cast<size_t>(max_staff + 1) * static_cast<size_t>(max_staff + 2) / 2;
  result.prob.resize(T * result.per_t);

  for (size_t t = 0; t < T; ++t) {
    double *pmf_t = result.prob.data() + t * result.per_t; // 每个t的起始位置，是一个指针
    for (int i = 0; i <= max_staff; ++i) {
      double *p = pmf_t + result.offset[i]; // 阶段t时，每个 i 的起始位置，是一个指针
      if (i == 0) {
        p[0] = 1.0;
      } else {
        for (int j = 0; j <= i; ++j) {
          p[j] = binomialPdf(i, j, ps[t]);
        }
      }
    }
  }
  return result;
}

int main() {
  int n = 10;
  int k = 3;
  double p = 0.4;

  std::cout << "P(X = " << k << ") = " << binomialPdf(n, k, p) << std::endl;

  return 0;
}