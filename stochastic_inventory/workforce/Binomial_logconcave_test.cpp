//
// Created by Administrator on 2025/7/16.
// Description: the tail probability and cdf of Binomial distribution are both log concave by the
// tests
//
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

// 计算组合数 C(n, k)
double binomial_coefficient(int n, int k) {
  if (k < 0 || k > n)
    return 0;
  if (k == 0 || k == n)
    return 1;
  double res = 1.0;
  for (int i = 1; i <= k; ++i) {
    res *= (n - i + 1);
    res /= i;
  }
  return res;
}

// 计算概率质量函数 P(B = k)
double binomial_pmf(int n, int k, double p) {
  double q = 1.0 - p;
  return binomial_coefficient(n, k) * pow(p, k) * pow(q, n - k);
}

// 计算尾概率 T(k) = P(B >= k)
vector<double> binomial_tail(int n, double p) {
  vector<double> tail(n + 1, 0.0);
  vector<double> pmf(n + 1, 0.0);
  for (int k = 0; k <= n; ++k) {
    pmf[k] = binomial_pmf(n, k, p);
  }
  double sum = 0.0;
  for (int k = n; k >= 0; --k) {
    sum += pmf[k];
    tail[k] = sum;
  }
  return tail;
}

// 计算累积分布函数 F(k) = P(B <= k)
vector<double> binomial_cdf(int n, double p) {
  vector<double> cdf(n + 1, 0.0);
  double cumulative = 0.0;
  for (int k = 0; k <= n; ++k) {
    cumulative += binomial_pmf(n, k, p);
    cdf[k] = cumulative;
  }
  return cdf;
}

// 检查是否 log-concave: T(k)^2 >= T(k-1)*T(k+1)
bool check_log_concavity(const vector<double> &T, int n, double p) {
  bool ok = true;
  for (int k = 1; k < n; ++k) {
    double left = T[k] * T[k];
    double right = T[k - 1] * T[k + 1];
    if (left < right - 1e-12) { // 容忍小数误差
      cout << fixed << setprecision(6);
      cout << "Violation: n=" << n << ", p=" << p << ", k=" << k << ", T(k)^2=" << left
           << ", T(k-1)*T(k+1)=" << right << endl;
      ok = false;
    }
  }
  return ok;
}

int main() {
  // vector<double> p_list = {0.1, 0.3, 0.5, 0.7, 0.9};
#pragma omp parallel for
  for (int n = 701; n <= 800; ++n) { // from 1 to 700
    for (double p = 0.01; p < 1.0; p = p + 0.01) {
      // auto T = binomial_tail(n, p);
      auto T = binomial_cdf(n, p);
      check_log_concavity(T, n, p);
    }
  }
  cout << "No counterexample found." << endl;
  return 0;
}
