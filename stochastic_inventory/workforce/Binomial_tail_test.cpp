//
// Created by Administrator on 2025/7/16.
// Description: the tail probability of Binomial distribution is not non-increasing by the tests
//
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
using namespace std;

#include <iostream>
#include <cmath>
#include <vector>
#include <iomanip>

using namespace std;

// log(n!) 精确计算
double log_factorial(int n) {
  static vector<double> cache(1, 0.0); // log(0!) = 0
  while ((int)cache.size() <= n) {
    cache.push_back(cache.back() + log(cache.size()));
  }
  return cache[n];
}

// log(C(n,k))
double log_binom(int n, int k) {
  if (k < 0 || k > n) return -INFINITY;
  return log_factorial(n) - log_factorial(k) - log_factorial(n - k);
}

// P(B = k)
double binom_pmf(int n, int k, double p) {
  if (k < 0 || k > n) return 0.0;
  double log_prob = log_binom(n, k) + k * log(p) + (n - k) * log(1 - p);
  return exp(log_prob);
}

// P(B >= k)
double binom_tail(int n, int k, double p) {
  double sum = 0.0;
  for (int i = k; i <= n; ++i) {
    sum += binom_pmf(n, i, p);
  }
  return sum;
}

int main() {
  // int N_max = 200;
  double p_min = 0.01, p_max = 0.99, p_step = 0.01;

#pragma omp parallel
  cout << fixed << setprecision(10);


  for (int n = 1; n <= 200; ++n) {  // from 1 to 500
    for (double p = p_min; p <= p_max; p += p_step) {
      // 预计算尾概率
      vector<double> T(n + 2);
      for (int k = 0; k <= n + 1; ++k) {
        T[k] = binom_tail(n, k, p);
      }

      // 检查 R(k) 是否递减
      for (int k = 0; k <= n - 2; ++k) {
        double Rk   = T[k + 1] / T[k];
        double Rk1  = T[k + 2] / T[k + 1];
        if (Rk < Rk1 - 1e-1) {  // 给一点浮点容差
          cout << "Counterexample found!" << endl;
          cout << "n = " << n << ", p = " << p << ", k = " << k << endl;
          cout << "R(" << k << ")   = " << Rk << endl;
          cout << "R(" << k+1 << ") = " << Rk1 << endl;
          // return 0;
        }
      }
    }
  }

  cout << "No counterexample found" << endl;
  return 0;
}
