/*
 * Created by Zhen Chen on 2026/7/31.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */
#include "two_product.h"
#include <boost/math/distributions/poisson.hpp>
#include <iostream>

namespace {
struct jointPMF {
  std::vector<int> supply_support;
  std::vector<int> demand1_support;
  std::vector<int> demand2_support;

  std::vector<double> prob_supply;
  std::vector<double> prob_demand1;
  std::vector<double> prob_demand2;

  std::vector<int> start_index_t; // start index of each t in flat arrays
  std::vector<int> len_t;         // length per t
};
} // namespace

jointPMF getPMF(const int supply, const int demand1, const int demand2,
                const double truncated_quantile, const int T) {
  const auto dist_supply =
      boost::math::poisson(supply); // poisson 是 poisson_distribution 的类型别名 typedef
  const auto dist_demand1 = boost::math::poisson(demand1);
  const auto dist_demand2 = boost::math::poisson(demand2);
  const int supply_lb =
      static_cast<int>(boost::math::quantile(dist_supply, 1 - truncated_quantile));
  const int supply_ub = static_cast<int>(boost::math::quantile(dist_supply, truncated_quantile));
  const int demand1_lb =
      static_cast<int>(boost::math::quantile(dist_demand1, 1 - truncated_quantile));
  const int demand1_ub = static_cast<int>(boost::math::quantile(dist_demand1, truncated_quantile));
  const int demand2_lb =
      static_cast<int>(boost::math::quantile(dist_demand1, 1 - truncated_quantile));
  const int demand2_ub = static_cast<int>(boost::math::quantile(dist_demand1, truncated_quantile));

  jointPMF pmf;
  const int supply_len = supply_ub - supply_lb + 1;
  const int demand1_len = demand1_ub - demand1_lb + 1;
  const int demand2_len = demand2_ub - demand2_lb + 1;
  pmf.supply_support.resize(supply_len);
  pmf.demand1_support.resize(demand1_len);
  pmf.demand2_support.resize(demand2_len);
  pmf.prob_supply.resize(supply_len);
  pmf.prob_demand1.resize(demand1_len);
  pmf.prob_demand2.resize(demand2_len);

  pmf.start_index_t.resize(T + 1, 0);
  const int total_len = supply_len * demand1_len * demand2_len;
  pmf.len_t.resize(T, total_len);

  // compute total size (for allocation)
  int total_size = 0;
  for (size_t t = 0; t < T; ++t) {
    pmf.start_index_t[t] = total_size;
    total_size += total_len;
  }
  pmf.start_index_t[T] = total_size;

  const double norm = 1.0 / (2.0 * truncated_quantile - 1.0);
  for (int i = 0; i < supply_len; ++i) {
    pmf.supply_support[i] = supply_lb + i;
    pmf.prob_supply[i] = boost::math::pdf(dist_supply, supply_lb + i) / norm;
  }
  for (int i = 0; i < demand1_len; ++i) {
    pmf.demand1_support[i] = demand1_lb + i;
    pmf.prob_demand1[i] = boost::math::pdf(dist_demand1, demand1_lb + i) / norm;
  }
  for (int i = 0; i < demand2_len; ++i) {
    pmf.demand2_support[i] = demand2_lb + i;
    pmf.prob_demand2[i] = boost::math::pdf(dist_demand2, demand2_lb + i) / norm;
  }
};

int main() {
  double h1 = 1.0; // unit holding cost
  double h2 = 2.0;
  double p1 = 2.0; // unit stock out cost
  double p2 = 3.0;

  int K = 50;                // fixed launching cost
  constexpr int mu = 10;     // yielding rate, follows Poisson distribution
  constexpr int lambda1 = 5; // demand rate, follows Poisson distribution
  constexpr int lambda2 = 8;
  constexpr double truncQuantile = 0.9999;
  constexpr double INF = 1e100;
  constexpr int T = 4;

  constexpr int min_I1 = -100;
  constexpr int max_I1 = 100;
  constexpr int min_I2 = 100;
  constexpr int max_I2 = 100;

  const auto start_time = std::chrono::high_resolution_clock::now();
  const auto pmf = getPMF(mu, lambda1, lambda2, truncQuantile, T);
  constexpr int num_inv1 = max_I1 - min_I1 + 1; // number of possible inventory levels for product 1
  constexpr int num_inv2 = max_I2 - min_I2 + 1; // number of possible inventory levels for product 2

  std::vector<double> hold_penalty_costs1(num_inv1);
  for (int i = 0; i < num_inv1; ++i) {
    const int inv = i + min_I1;
    hold_penalty_costs1[i] = (inv > 0) ? h1 * inv : -p1 * inv;
  }
  std::vector<double> hold_penalty_costs2(num_inv2);
  for (int i = 0; i < num_inv2; ++i) {
    const int inv = i + min_I2;
    hold_penalty_costs2[i] = (inv > 0) ? h2 * inv : -p2 * inv;
  }

  //----------------------------------------------------
  // backward DP
  // ----------------------------------------------------
  // 1D vector can be faster. This is key to speed up the computation
  std::vector value((T + 1) * num_inv1 * num_inv2, 0.0);
  std::vector policy(T * num_inv1 * num_inv2, 0);

  // V 是value首元素指针
  // *V 就是对应的值，等同于 V[0]
  // 用指针访问数组并不能显著提升运算速度
  double *V = value.data(); // Returns a pointer to the underlying array serving as element storage
  int *P = policy.data();
  const int *supply = pmf.supply_support.data();
  const int *demand1 = pmf.demand1_support.data();
  const int *demand2 = pmf.demand2_support.data();
  const double *prob1 = pmf.prob_demand1.data();
  const double *prob2 = pmf.prob_demand2.data();
  const double *prob_supply = pmf.prob_supply.data();

  const int *offset = pmf.offset.data();
  const double *H = hold_penalty_costs.data();

  // 使用 Lambda 表达式定义在 main 内部
  // [&] 或 [num_inv2] 表示捕获外部的 num_inv2 变量
  // 返回一维数组对应的索引
  auto getIndex = [&](const int i1, const int i2) { return i1 * num_inv2 + i2; };

  const auto end_time = std::chrono::high_resolution_clock::now();
  const auto elapsed = end_time - start_time;
  std::cout << "planning horizon = " << T << '\n';
  std::cout << "running time = " << elapsed.count() << " seconds\n";

  return 0;
}