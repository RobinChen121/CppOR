/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 15/08/2025, 17:58
 * Description: The codes is to implement the heuristic in:
 *  Bollapragada S, Morton T E. A simple heuristic for computing nonstationary (s, S) policies[J].
 * Operations research, 1999, 47(4): 576-584. to compute the values of (s, S).
 *
 */
#include "../../utils/pmf.h"

#include <cmath> // for NAN
#include <iostream>
#include <vector>

class InfiniteStationary {
  std::vector<double> m;     // m[j]
  std::vector<double> M;     // M[j]
  std::vector<double> probs; // 概率序列
  std::vector<double> G;     // G[y]
  int mean_demand;
  int max_demand;
  double m0; // m(0) 初值
  double M0; // M(0) 初值
  double h;
  double pi;
  double K;

public:
  // 构造函数
  explicit InfiniteStationary(const int mean_demand, const int max_demand, const double h,
                              const double pi, const double K)
      : mean_demand(mean_demand), max_demand(max_demand), h(h), pi(pi), K(K) {
    compute_probs();

    m0 = 1.0 / (1.0 - probs[0]); // m(0) = 1/(1-p0)
    M0 = 0.0;                    // M(0) = 0

    // 初始化数组为 NAN，表示未计算
    m.resize(max_demand, NAN);
    M.resize(max_demand, NAN);
    G.resize(max_demand, NAN);
  }

  void compute_probs() {
    probs.resize(max_demand);
    for (int i = 0; i < max_demand; i++) {
      probs[i] = PMF::poissonPMF(i, mean_demand);
    }
  }

  std::array<int, 2> compute_sS() {
    const double critical_ratio = pi / (pi + h);
    const int y_star = PMF::poissonQuantile(critical_ratio, mean_demand);
    int S0 = y_star;
    int s;
    for (int s = y_star; s >= 0; s--) {
      if (compute_c(s, S0) <= compute_G(s))
        break;
    }
    auto s0 = s;
    auto c0 = compute_c(s, S0);
    auto S = S0 + 1;
    while (compute_G(S) <= c0) {
      if (compute_c(S, S) < c0) {
        S0 = S;
        while (compute_c(s, S0) <= compute_G(s + 1)) {
          s += 1;
          c0 = compute_c(s, S0);
        }
      }
      S = S + 1;
    }
    return {s, S};
  }

  double compute_c(const int s, const int S) { return compute_k(s, S) / compute_M(S - s); }

  // 按需计算 G(y)
  double compute_G(const int y) {
    if (y < G.size() && !std::isnan(G[y]))
      return G[y];

    if (y >= G.size())
      G.resize(y + 1, NAN);

    double overage_cost = 0.0;  // for h
    double underage_cost = 0.0; // for pi
    for (int d = 0; d < max_demand; ++d) {
      if (d < y)
        overage_cost += h * (y - d) * probs[d];
      else
        underage_cost += pi * (d - y) * probs[d];
    }
    G[y] = overage_cost + underage_cost;
    return G[y];
  }

  // 计算 k(s, y)
  double compute_k(const int s, const int y) {
    double mG = 0;
    for (int j = 0; j <= y - s - 1; ++j) {
      mG += compute_G(y - j) * compute_m(j);
    }
    return mG;
  }

  // 按需计算 m[j]
  double compute_m(const int j) {
    if (j < m.size() && !std::isnan(m[j]))
      return m[j];

    if (j >= m.size())
      m.resize(j + 1, NAN);

    if (j == 0) {
      m[0] = m0;
      return m[0];
    }

    double sum = 0.0;
    for (int l = 1; l <= j && l < probs.size(); l++) {
      sum += probs[l] * compute_m(j - l);
    }

    m[j] = sum / (1.0 - probs[0]);
    return m[j];
  }

  // 按需计算 M[j]
  double compute_M(const int j) {
    if (j < M.size() && !std::isnan(M[j]))
      return M[j];

    if (j >= M.size())
      M.resize(j + 1, NAN);

    if (j == 0) {
      M[0] = M0;
      return M[0];
    }

    M[j] = compute_M(j - 1) + compute_m(j - 1);
    return M[j];
  }

  // 可选：打印前 N 个值
  void print(const int N) {
    for (int j = 0; j <= N; j++) {
      std::cout << "m(" << j << ") = " << compute_m(j) << ", M(" << j << ") = " << compute_M(j)
                << std::endl;
    }
  }
};

int main() {
  constexpr int max_demand = 100;
  const double h = 1;
  const double pi = 2;
  const double K = 10;
  constexpr int mean_demand = 10;
  std::vector<double> probs(max_demand, 0.0);
  for (int i = 0; i < max_demand; i++) {
    probs[i] = PMF::poissonPMF(i, mean_demand);
  }

  InfiniteStationary problem(mean_demand, max_demand, h, pi, K);

  problem.print(max_demand);

  return 0;
}
