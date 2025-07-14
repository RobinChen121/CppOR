//
// Created by Administrator on 2025/7/14.
//
#include <cmath>
#include <iomanip>
#include <iostream>
#include <omp.h>
#include <vector>

// 计算组合数 C(n, k) 用动态规划（防止溢出用double）
double comb(const int n, int k) {
  if (k > n)
    return 0;
  if (k > n - k)
    k = n - k;
  double res = 1.0;
  for (int i = 1; i <= k; ++i) {
    res *= (n - k + i);
    res /= i;
  }
  return res;
}

// 计算二项分布概率 P(B = k)
double binomial_prob(const int n, const double p, const int k) {
  return comb(n, k) * std::pow(p, k) * std::pow(1 - p, n - k);
}

// 计算尾概率 P(B > j)
double tail_prob(const int n, const double p, const int j) {
  double sum = 0.0;
  for (int k = j + 1; k <= n; ++k) {
    sum += binomial_prob(n, p, k);
  }
  return sum;
}

// 计算期望 E[B] = n * p
double expectation(const int n, double p) { return n * p; }

int main() {
  std::cout << std::fixed << std::setprecision(6);

  bool found = false;

  // 尝试不同的 n, p 和 j
#pragma omp parallel for
  for (int n = 341; n <= 350; ++n) {              // 试验次数1~350
    for (double p = 0.01; p <= 0.99; p += 0.01) { // p从0.01到0.99步长0.01
      const double E = expectation(n, p);

      for (int j = 0; j < n; ++j) {
        const double P_j = tail_prob(n, p, j);
        const double lhs = E * P_j;

        double rhs = 0.0;
        for (int k = j + 1; k <= n; ++k) {
          rhs += tail_prob(n, p, k);
        }

        if (lhs < rhs - 1e-12) { // 允许极小数值误差
          std::cout << "counterexample: n=" << n << ", p=" << p << ", j=" << j << "\n";
          std::cout << "expectation E=" << E << "\n";
          std::cout << "P(B>" << j << ")=" << P_j << "\n";
          std::cout << "left-hand side E*P(B>" << j << ")=" << lhs << "\n";
          std::cout << "right-hand side sum_{k=" << j + 1 << "}^" << n << " P(B>k)=" << rhs << "\n";
          found = true;
          break;
        }
      }
    }
  }
  if (!found) {
    std::cout << "no counterexample found\n";
  }
  return 0;
}
