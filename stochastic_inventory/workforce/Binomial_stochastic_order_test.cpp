//
// Created by Administrator on 2025/7/15.
//
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

// 使用对数阶乘计算二项式系数的对数值以避免溢出
double log_binomial_coeff(int n, int k) {
  return lgamma(n + 1) - lgamma(k + 1) - lgamma(n - k + 1);
}

// 二项分布概率质量函数（精确）
double binomial_pmf(int n, int k, double p) {
  if (k < 0 || k > n)
    return 0.0;
  const double log_prob = log_binomial_coeff(n, k) + k * log(p) + (n - k) * log(1 - p);
  return exp(log_prob);
}

// 计算 Pr(X > t) 给定二项分布参数
double tail_prob_binomial(const int n, const double p, const int t) {
  double sum = 0.0;
  for (int k = t + 1; k <= n; ++k)
    sum += binomial_pmf(n, k, p);
  return sum;
}

// 计算 Pr(B > t | B > j)
double conditional_tail_prob(const int b, const double p, const int j, const int t) {
  double num = 0.0, denom = 0.0;
  for (int k = j + 1; k <= b; ++k)
    denom += binomial_pmf(b, k, p);
  for (int k = t + 1; k <= b; ++k)
    num += binomial_pmf(b, k, p);
  return denom > 0 ? num / denom : 0.0;
}

int main() {
  cout << fixed << setprecision(5);
  double eps = 1e-5;

#pragma omp parallel for
  for (int b = 191; b <= 200; ++b) { // from 1 to 200
    for (double p = 0.1; p < 1.0; p += 0.01) {
      for (int j = 0; j < b - 1; ++j) {
        int b2 = b - j - 1;
        for (int t = j + 1; t <= b; ++t) {
          const double cond_prob = conditional_tail_prob(b, p, j, t);
          double bound_prob = tail_prob_binomial(b2, p, t - (j + 1));
          if (cond_prob > bound_prob + eps) {
            cout << "❗ Violation: b=" << b << ", j=" << j << ", p=" << p << ", t=" << t
                 << " | P(B > t | B > j)=" << cond_prob << " > P(B' > t)=" << bound_prob << endl;
          }
        }
      }
    }
  }

  cout << "no counterexample found!" << endl;
  return 0;
}
