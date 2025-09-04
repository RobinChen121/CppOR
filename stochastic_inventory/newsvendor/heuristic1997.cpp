/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 15/08/2025, 17:58
 * Description: The codes is to implement the heuristic in:
 *  Bollapragada S, Morton T E. A simple heuristic for computing non-stationary (s, S) policies[J].
 * Operations research, 1999, 47(4): 576-584. to compute the values of (s, S).
 *
 */
#include "../../utils/pmf.h"

#include <array>
#include <cmath> // for NAN
#include <iostream>
#include <vector>

class ComputesS {
  std::vector<std::vector<double> > m; // m[j]
  std::vector<std::vector<double> > M; // M[j]
  std::vector<std::vector<std::vector<double> > > probs; // 概率序列
  std::vector<std::vector<double> > G; // G[y]
  std::vector<double> demands;
  double M0; // M(0) 初值
  double h;
  double pi;
  double K;
  size_t T;

public:
  // 构造函数
  explicit ComputesS(const std::vector<std::vector<std::vector<double> > > &probs,
                     const double h,
                     const double pi, const double K)
    : probs(probs), h(h), pi(pi), K(K) {

    M0 = 0.0; // M(0) = 0

    T = probs.size();

    m.resize(T);
    M.resize(T);
    G.resize(T);
    for (int t = 0; t < T; ++t) {
      m[t].resize(probs[t].size(), NAN);
      M[t].resize(probs[t].size(), NAN);
      G[t].resize(probs[t].size(), NAN);
    }
  }

  double compute_k(int s, int y, int t);

  std::array<int, 2> computeStationary_sS(int t);

  double compute_c(const int s, const int S, const int t) {
    return compute_k(s, S, t) / compute_M(S - s, t);
  }

  std::vector<std::array<int, 2> > computeMulti_sS();

  double compute_M(int j, int t);

  double compute_G(int y, int t);

  double compute_m(int j, int t);

};

// 计算 k(s, y)
double ComputesS::compute_k(const int s, const int y, const int t) {
  double mG = K;
  for (int j = 0; j <= y - s - 1; ++j) {
    mG += compute_G(y - j, t) * compute_m(j, t);
  }
  return mG;
}

std::array<int, 2> ComputesS::computeStationary_sS(const int t) {
  const double critical_ratio = pi / (pi + h);
  double sum_prob = 0.0;
  int y_star = 0;
  for (auto element : probs[t]) {
    sum_prob += element[1];
    if (sum_prob > critical_ratio) {
      y_star = static_cast<int>(element[0]);
      break;
    }
  }
  int S0 = y_star;
  int s;
  // 首先算出 y* 对应的 s, 这个 s 是 s* 的下界
  for (s = y_star; s >= 0; s--) {
    if (compute_c(s, S0, t) <= compute_G(s, t))
      break;
  }
  const auto s0 = s;
  auto c0 = compute_c(s0, S0, t);
  auto S = S0 + 1;
  while (compute_G(S, t) <= c0) {
    // G(S*) 一定小于等于 c0
    if (compute_c(s, S, t) < c0) {
      // 此时更新 S，因为成本在改善
      S0 = S;
      // 计算更新后 S 对应的 s
      while (compute_c(s, S0, t) <= compute_G(s + 1, t)) {
        s += 1;
      }
      c0 = compute_c(s, S0, t);
      S = S + 1;
    } else {
      S--;
      break;
    }
  }
  return {s, S};
}


std::vector<std::array<int, 2> > ComputesS::computeMulti_sS() {
  std::vector<std::array<int, 2> > arr_sS(T);
  for (int i = 0; i < T; i++) {
    arr_sS[i] = computeStationary_sS(i);
  }
  return arr_sS;
}


// 按需计算 M[j]
// NOLINTNEXTLINE(misc-no-recursion)
double ComputesS::compute_M(const int j, const int t) {
  if (j >= M[t].size())
    M[t].resize(j + 1, NAN);
  M[t][j] = 0.0;
  if (j > 1) {
    for (int i = 0; i < j; i++)
      M[t][j] += compute_m(i, t);
  }
  return M[t][j];
}

// 按需计算 G(y)
double ComputesS::compute_G(const int y, const int t) {
  if (y < G[t].size() && !std::isnan(G[t][y]))
    return G[t][y];
  if (y >= G[t].size())
    G[t].resize(y + 1, NAN);

  double overage_cost = 0.0; // for h
  double underage_cost = 0.0; // for pi
  const int max_demand = static_cast<int>(G[t].size()) - 1;
  for (int d = 0; d < max_demand; ++d) {
    if (d < y)
      overage_cost += h * (y - d) * probs[t][d][1];
    else
      underage_cost += pi * (d - y) * probs[t][d][1];
  }
  G[t][y] = overage_cost + underage_cost;
  return G[t][y];
}

// 按需计算 m[j]
// 有问题
// NOLINTNEXTLINE(misc-no-recursion)
double ComputesS::compute_m(const int j, const int t) {
  if (j < m[t].size() && !std::isnan(m[t][j]))
    return m[t][j];

  if (j >= m.size())
    m[t].resize(j + 1, NAN);

  if (j == 0) {
    m[t][0] = 1.0 / (1.0 - probs[t][0][1]); // m(0) = 1/(1-p0);
    return m[t][0];
  }

  double sum = 0.0;
  for (int l = 1; l <= j && l < probs[t].size(); l++) {
    sum += probs[t][l][1] * compute_m(j - l, t);
  }

  m[t][j] = sum / (1.0 - probs[t][0][1]);
  return m[t][j];
}

int main() {
  // optimal s, S should be 15, 66
  constexpr double h = 1;
  constexpr double pi = 9;
  constexpr double K = 64;
  constexpr int mean_demand = 15;
  constexpr double rho = 0.4;
  constexpr double truncate_quantile = 0.9999;
  constexpr double step_size = 1.0;
  constexpr int T = 10;
  const std::vector<double> demands(T, mean_demand);
  const std::vector<std::vector<std::vector<double> > > probs = PMF(truncate_quantile, step_size).
      getPMFPoisson(demands);

  ComputesS problem(probs, h, pi, K);
  const auto sS = problem.computeMulti_sS();

  std::cout << "The values of sS are: " << std::endl;
  for (auto row : sS) {
    for (const auto col : row) {
      std::cout << col << " ";
    }
    std::cout << std::endl;
  }

  std::vector<double> M_values(T);
  std::cout << "The M(S-s) for each period are " << std::endl;
  for (int t = 0; t < T; t++) {
    M_values[t] = problem.compute_M(sS[t][1] - sS[t][0], t);
    std::cout << M_values[t] << std::endl;
  }

  return 0;
}