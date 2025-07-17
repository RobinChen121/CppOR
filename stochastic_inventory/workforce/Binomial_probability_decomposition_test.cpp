/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 2025/7/17, 11:25
 * Description: 
 * 
 */
#include <iostream>
#include <iomanip>
#include <cmath>

// 计算二项系数 C(n, k)
double binomial_coefficient(int n, int k) {
  if (k < 0 || k > n) return 0.0;
  if (k > n - k) k = n - k;
  double res = 1.0;
  for (int i = 1; i <= k; ++i) {
    res *= (n - k + i);
    res /= i;
  }
  return res;
}

// 计算二项分布尾概率 P[X >= k]
double binomial_tail(int n, int k, double p) {
  if (k > n) return 0.0;
  double prob = 0.0;
  for (int i = k; i <= n; ++i) {
    double term = binomial_coefficient(n, i) * std::pow(p, i) * std::pow(1-p, n - i);
    prob += term;
  }
  return prob;
}

int main() {
  std::cout << std::fixed << std::setprecision(6);

  bool found_counterexample = false;

  // 遍历 n 从 1 到 250
#pragma omp parallel for
  for (int n = 241; n <= 250; ++n) {
    // 遍历 p 从 0.01 到 0.99，步长 0.01
    for (double p = 0.01; p < 1.0; p += 0.01) {
      // 遍历 a, b
      for (int a = 0; a <= n; ++a) {
        for (int b = 0; b <= n; ++b) {
          int c = a + b;
          if (c > n) continue;

          double lhs = binomial_tail(n, c, p);
          double rhs = binomial_tail(n, a, p) * binomial_tail(n, b, p);

          if (lhs > rhs + 1e-3) {
            std::cout << "Counterexample found: n=" << n << ", p=" << p
                      << ", a=" << a << ", b=" << b << "\n";
            std::cout << "P[X >= a+b] = " << lhs << ", P[X >= a]*P[X >= b] = " << rhs << "\n\n";
            found_counterexample = true;

          }
        }
      }
    }
  }

  if (!found_counterexample) {
    std::cout << "No counterexample found in given parameter ranges.\n";
  }

  return 0;
}

