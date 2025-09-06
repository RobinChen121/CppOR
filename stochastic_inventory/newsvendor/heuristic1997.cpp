/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 15/08/2025, 17:58
 * Description: The codes is to implement the heuristic in:
 *  Bollapragada S, Morton T E. A simple heuristic for computing non-stationary (s, S) policies[J].
 * Operations research, 1999, 47(4): 576-584. to compute the values of (s, S).
 *
 * This method along with the paper in 1991(Zheng and Federgruen) seem to be affected by the float
 * precision, making them result in larger gaps of s, S when the mean demand is large
 *
 */
#include "../../utils/pmf.h"

#include <array>
#include <cmath> // for NAN
#include <iostream>
#include <vector>
#include <unordered_map>


// 1. 定义结构体
struct Key {
  int x;
  int y;
  int z;

  // 相等比较函数（unordered_map 需要）
  bool operator==(const Key &other) const {
    return x == other.x && y == other.y && z == other.z;
  }
};

// 2. 定义自定义哈希函数
struct KeyHash {
  std::size_t operator()(const Key &k) const {
    std::size_t h1 = std::hash<int>{}(k.x);
    std::size_t h2 = std::hash<int>{}(k.y);
    std::size_t h3 = std::hash<int>{}(k.z);

    // hash_combine 技巧
    std::size_t seed = h1;
    seed ^= h2 + 0x9e3779b97f4a7c15 + (seed << 6) + (seed >> 2);
    seed ^= h3 + 0x9e3779b97f4a7c15 + (seed << 6) + (seed >> 2);
    return seed;
  }
};

class ComputesS {
  std::vector<std::vector<double>> m;                  // m[j]
  std::vector<std::vector<double>> M;                  // M[j]
  std::vector<std::vector<std::vector<double>>> probs; // 概率序列
  std::vector<std::vector<double>> G;                  // G[y]
  std::vector<double> demands;
  double M0; // M(0) 初值
  double h;
  double pi;
  double K;
  size_t T;
  int min_s;

  // 3. 定义 unordered_map，Key 为键，std::string 为值
  std::unordered_map<Key, double, KeyHash> myMap;

public:
  // 构造函数
  explicit ComputesS(const std::vector<std::vector<std::vector<double>>> &probs, const double h,
                     const double pi, const double K, const std::vector<double> &demands)
      : probs(probs), h(h), pi(pi), K(K), demands(demands) {

    M0 = 0.0; // M(0) = 0

    T = probs.size();
    min_s = -10;

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

  std::vector<std::array<int, 2>> computeMulti_sS();

  double compute_M(int j, int t);

  double compute_G(int y, int t);

  double compute_m(int j, int t);
};

std::array<int, 2> ComputesS::computeStationary_sS(const int t) {
  const double critical_ratio = pi / (pi + h);
  double sum_prob = 0.0;
  // int y_star = PMF::poissonQuantile(critical_ratio, demands[t]);
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
  for (s = y_star; s >= min_s; s--) {
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

std::vector<std::array<int, 2>> ComputesS::computeMulti_sS() {
  std::vector<std::array<int, 2>> arr_sS(T);
  for (int i = 0; i < T; i++) {
    arr_sS[i] = computeStationary_sS(i);
  }
  return arr_sS;
}

// 计算 k(s, y)
// NOLINTNEXTLINE(misc-no-recursion)
double ComputesS::compute_k(const int s, const int y, const int t) {
  // double sum_p = 0.0;
  // for (int i = y - s; i < probs[t].size(); i++) {
  //   sum_p += probs[t][i][1];
  // }
  // const Key key{s, y, t};
  //
  // if (y == s + 1) {
  //   myMap[key] = (compute_G(y, t) + K * sum_p) / (1 - probs[t][0][1]);
  // }
  //
  // double sum_pk = 0.0;
  // for (int j = 1; j <= y - s - 1; j++) {
  //   if (myMap.contains({s, y - j, t}))
  //     sum_pk += probs[t][j][1] * myMap[{s, y - j, t}];
  //   else
  //     sum_pk += probs[t][j][1] * compute_k(s, y - j, t);
  // }
  // const double final_value = (compute_G(y, t) + K * sum_p + sum_pk) / (1 - probs[t][0][1]);
  // myMap[key] = final_value;
  // return final_value;

  // 下面这种方法的计算精度会更高些，尤其是在正态分布时
  double mG = K;
  for (int j = 0; j <= y - s - 1; ++j) {
    mG += compute_G(y - j, t) * compute_m(j, t);
  }
  return mG;
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
  min_s = 0;
  if (y - min_s < G[t].size() && !std::isnan(G[t][y - min_s]))
    return G[t][y - min_s];
  if (y - min_s >= G[t].size())
    // resize 要么截断容器，后面的元素会被销毁
    // 要么扩展容器，新元素会被加到末尾，并用默认值填充
    G[t].resize(y - min_s + 1, NAN);

  double overage_cost = 0.0;  // for h
  double underage_cost = 0.0; // for pi

  for (int d = 0; d < probs[t].size(); ++d) {
    const double p = probs[t][d][1];
    if (d < y) {
      overage_cost += h * (y - d) * p;
    } else {
      underage_cost += pi * (d - y) * p;
    }
  }
  G[t][y - min_s] = overage_cost + underage_cost;
  return G[t][y - min_s];
}

// 按需计算 m[j]
// 有问题
// NOLINTNEXTLINE(misc-no-recursion)
double ComputesS::compute_m(const int j, const int t) {
  const int min_demand = static_cast<int>(probs[t][0][0]);
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
  constexpr double h = 1;
  constexpr double pi = 5;
  constexpr double K = 80;
  constexpr int mean_demand = 10;
  constexpr double rho = 0.4;
  constexpr double truncate_quantile = 0.9999; // 这个还挺影响s, S 的具体值的
  constexpr double step_size = 1.0;
  constexpr int T = 1;
  const std::vector<double> demands(T, mean_demand);
  std::vector<double> sigmas(T);
  for (int t = 0; t < T; ++t) {
    sigmas[t] = rho * demands[t];
  }
  const std::vector<std::vector<std::vector<double>>> probs = PMF(truncate_quantile, step_size)
                                                                  .
                                                              getPMFNormal(demands, sigmas);
                                                              // getPMFPoisson(demands);

  // int max_demand = 100;
  // std::vector<std::vector<std::vector<double> > > probs;
  // probs.resize(T);
  // int min_demand = 0;
  // probs[0].resize(max_demand + 1);
  // for (int i = 0; i < max_demand + 1; i++) {
  //   probs[0][i].resize(2);
  //   probs[0][i][0] = i;
  //   probs[0][i][1] = PMF::poissonPMF(i, mean_demand);
  //   // , rho*mean_demand,
  //   // truncate_quantile);
  // }

  ComputesS problem(probs, h, pi, K, demands);
  // double v1 = problem.compute_k(10, 20, 0);

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

  std::vector<double> c_values(T);
  std::cout << "The c(S-s) for each period are " << std::endl;
  for (int t = 0; t < T; t++) {
    c_values[t] = problem.compute_c(sS[t][0], sS[t][1], t);
    std::cout << c_values[t] << std::endl;
  }

  return 0;
}