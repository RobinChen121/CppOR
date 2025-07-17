//
// Created by Administrator on 2025/7/14.
//
#include <iostream>
#include <cmath>
#include <iomanip>

using namespace std;

// 计算组合数 C(n, k)
double binomial_coeff(int n, int k) {
  if (k < 0 || k > n) return 0;
  double res = 1;
  for (int i = 1; i <= k; ++i)
    res *= static_cast<double>(n - i + 1) / i;
  return res;
}

// 计算 P(X = k) for Bin(b, p)
double binomial_prob(const int b, const double p, const int k) {
  return binomial_coeff(b, k) * pow(p, k) * pow(1 - p, b - k);
}

int main() {
  cout << fixed << setprecision(4);
#pragma omp parallel for
  for (int b = 501; b <= 520; ++b) { // 1-520
    for (double p = 0.01; p < 1.0; p += 0.01) {
      const double bp = b * p;
      for (int j = 0; j < b; ++j) {
        double numerator = 0.0;
        double denom = 0.0;

        for (int k = j + 1; k <= b; ++k) {
          const double pk = binomial_prob(b, p, k);
          numerator += k * pk;
          denom += pk;
        }

        if (denom == 0) continue; // avoid division by 0

        const double cond_expect = numerator / denom;
        double lhs = cond_expect - j - 1;

        if (lhs > bp + 1e-6) { // 允许一点浮点误差
          cout << "Counterexample found:\n";
          cout << "b = " << b << ", p = " << p << ", j = " << j << "\n";
          cout << "E[X | X > j] = " << cond_expect << "\n";
          cout << "E[X | X > j] - j - 1 = " << lhs << " > bp = " << bp << "\n\n";
        }
      }
    }
  }
  cout << "Counterexample not found:\n";
  return 0;
}
