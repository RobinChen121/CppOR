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

#include <array>
#include <cmath> // for NAN
#include <iostream>
#include <vector>

class InfiniteStationary {
  std::vector<double> m;     // m[j]
  std::vector<double> M;     // M[j]
  std::vector<double> probs; // 概率序列
  std::vector<double> G;     // G[y]
  std::vector<double> demands;
  int max_demand;
  double M0; // M(0) 初值
  double h;
  double pi;
  double K;

public:
  // 构造函数
  explicit InfiniteStationary(const std::vector<double>& probs, const std::vector<double>& demands, const int max_demand, const double h,
                              const double pi, const double K)
      : probs(probs), demands(demands), max_demand(max_demand), h(h), pi(pi), K(K) {

    M0 = 0.0;                    // M(0) = 0

    m.resize(max_demand, NAN);
    M.resize(max_demand, NAN);
    G.resize(max_demand, NAN);
  }

  std::array<int, 2> computeSinglesS(const double mean_demand) {
    const double critical_ratio = pi / (pi + h);
    const int y_star = PMF::poissonQuantile(critical_ratio, mean_demand);
    int S0 = y_star;
    int s;
    // 首先算出 y* 对应的 s, 这个 s 是 s* 的下界
    for (s = y_star; s >= 0; s--) {
      if (compute_c(s, S0) <= compute_G(s))
        break;
    }
    const auto s0 = s;
    auto c0 = compute_c(s0, S0);
    auto S = S0 + 1;
    while (compute_G(S) <= c0) {
      // G(S*) 一定小于等于 c0
      if (compute_c(s, S) < c0) { // 此时更新 S，因为成本在改善
        S0 = S;
        // 计算更新后 S 对应的 s
        while (compute_c(s, S0) <= compute_G(s + 1)) {
          s += 1;
        }
        c0 = compute_c(s, S0);
        S = S + 1;
      }
      else{
        S--;
        break;
    }
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
    double mG = K;
    for (int j = 0; j <= y - s - 1; ++j) {
      mG += compute_G(y - j) * compute_m(j);
    }
    return mG;
  }

  // 按需计算 m[j]
  // 有问题
  // NOLINTNEXTLINE(misc-no-recursion)
  double compute_m(const int j) {
    if (j < m.size() && !std::isnan(m[j]))
      return m[j];

    if (j >= m.size())
      m.resize(j + 1, NAN);

    if (j == 0) {
      m[0] =  1.0 / (1.0 - probs[0]); // m(0) = 1/(1-p0);
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
  // NOLINTNEXTLINE(misc-no-recursion)
  double compute_M(const int j) {
    M.resize(j + 1, NAN);
    M[j] = 0.0;
    if (j > 1) {
      for (int i = 0; i < j; i++)
        M[j] += compute_m(i);
    }
    return M[j];
  }

  std::array<double, 2> computesS() {
    size_t T = demands.size();
    size_t sS(T);
    return {};
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
  // optimal s, S should be 15, 66
  constexpr int max_demand = 50;
  constexpr double h = 1;
  constexpr double pi = 9;
  constexpr double K = 64;
  constexpr int mean_demand = 15;
  constexpr double rho = 0.4;
  constexpr double truncate_quantile = 0.9999;
  std::vector<double> probs(max_demand);
  for (int i = 0; i < max_demand; i++) {
    probs[i] = PMF::poissonPMF(i, mean_demand);
    // probs[i] = PMF::normalPMF(i, mean_demand, rho*mean_demand, truncate_quantile);
  }

  constexpr int T = 10;
  const std:: vector<double> demands(T, mean_demand);

  InfiniteStationary problem(probs, demands, max_demand, h, pi, K);
  const auto sS = problem.computeSinglesS(demands[0]);

  std::cout << "c* is: " << problem.compute_c(sS[0], sS[1]) << std::endl;
  std::cout << "The values of sS are: " << std::endl;
  std::cout << "s is " << sS[0] << ", S is " << sS[1] << std::endl;
  std::cout << "M(S-s) is " << std::endl;
  std::cout << problem.compute_M(sS[1] - sS[0]-1) << std::endl;

  return 0;
}
