/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 06/06/2026, 10:59
 * Description: using 2D vector to compute the DP, which is faster than using unordered_map.
 * using 2D vector is 0.077s;
 * using 1D vector can be much faster 0.056s;
 * using pointer to access vector values can be faster further 0.04s.
 * 40 periods, running time under serial computing is 0.035s(dell), a little faster than Julia
 * 0.048s
 *
 *
 */

#include <boost/math/distributions/poisson.hpp>
#include <chrono>
#include <iostream>
#include <limits>
#include <vector>

struct PMFData {
  std::vector<int> demand;  // flattened demand
  std::vector<double> prob; // flattened probability
  std::vector<int> offset;  // start index of each t in flat arrays
  std::vector<int> len;     // length per t
};

PMFData getPMFPoisson(const std::vector<double> &demands, const double truncated_quantile) {
  const size_t T = demands.size();

  std::vector<int> support_lb(T);
  std::vector<int> support_ub(T);

  // compute bounds
  for (size_t t = 0; t < T; ++t) {
    boost::math::poisson_distribution<> dist(demands[t]);
    support_lb[t] = static_cast<int>(boost::math::quantile(dist, 1 - truncated_quantile));
    support_ub[t] = static_cast<int>(boost::math::quantile(dist, truncated_quantile));
  }

  PMFData pmf;

  pmf.offset.resize(T + 1, 0);
  pmf.len.resize(T);

  // compute total size (for allocation)
  int total_size = 0;
  for (size_t t = 0; t < T; ++t) {
    pmf.len[t] = support_ub[t] - support_lb[t] + 1;
    pmf.offset[t] = total_size;
    total_size += pmf.len[t];
  }
  pmf.offset[T] = total_size;

  pmf.demand.resize(total_size);
  pmf.prob.resize(total_size);

  // fill flattened arrays
  for (size_t t = 0; t < T; ++t) {
    boost::math::poisson_distribution<> dist(demands[t]);

    const int base = pmf.offset[t];
    const int len = pmf.len[t];
    const int lb = support_lb[t];

    const double norm = 1.0 / (2.0 * truncated_quantile - 1.0);

    for (int j = 0; j < len; ++j) {
      int demand = lb + j;
      const int idx = base + j;

      pmf.demand[idx] = demand;
      pmf.prob[idx] = boost::math::pdf(dist, demand) * norm;
    }
  }

  return pmf;
}

int main() {
  constexpr int T = 2;
  constexpr double mean_demand = 10.0;
  const std::vector demands(T, mean_demand);

  constexpr int capacity = 150;
  constexpr double fix_order_cost = 0.0;
  constexpr double unit_order_cost = 1.0;
  constexpr double hold_cost = 2.0;
  constexpr double penalty_cost = 10.0;
  constexpr double truncQuantile = 0.9999;
  constexpr double INF = 1e100;

  constexpr int min_I = -100;
  constexpr int max_I = 100;
  constexpr int num_inv = max_I - min_I + 1;

  const auto start_time = std::chrono::high_resolution_clock::now();
  const auto pmf = getPMFPoisson(demands, truncQuantile);

  //------------------------------------------------------------------
  // value[t][i]
  // t = 0,...,T
  // inventory = i + min_I
  //------------------------------------------------------------------

  std::vector<double> hold_penalty_costs(num_inv);
  for (int i = 0; i < num_inv; ++i) {
    const int inv = i + min_I;
    hold_penalty_costs[i] = (inv > 0) ? hold_cost * inv : -penalty_cost * inv;
  }

  //----------------------------------------------------
  // backward DP
  // //----------------------------------------------------
  // 1D vector can be faster
  std::vector value((T + 1) * num_inv, 0.0);
  std::vector policy(T * num_inv, 0);

  // V 是value首元素指针
  // *V 就是对应的值，等同于 V[0]
  double *V = value.data(); // Returns a pointer to the underlying array serving as element storage
  int *P = policy.data();
  const int *demand = pmf.demand.data();
  const double *prob = pmf.prob.data();
  const int *offset = pmf.offset.data();
  const double *H = hold_penalty_costs.data();

  // =========================
  // DP CORE (HOT LOOP)
  // =========================
  for (int t = T - 1; t >= 0; --t) {

    const int base_t = t * num_inv;
    const int base_tp = (t + 1) * num_inv;
    const int start = offset[t];
    const int end = offset[t + 1];

    // 指针可以加减，相当于移动位置
    // 下面相当于把各阶段的 V与 P 提出来了
    double *V_current = V + base_t;
    double *V_next = V + base_tp;
    int *P_current = P + base_t;

    for (int idx = 0; idx < num_inv; ++idx) {
      int inventory = idx + min_I;
      double best_value = INF;
      int best_action = 0;

      for (int action = 0; action <= capacity; ++action) {
        const double fix_cost = (action > 0) ? fix_order_cost : 0.0;
        const double variable_cost = action * unit_order_cost;
        const int inventory_up_level = inventory + action;

        double current_value = 0.0;
        for (int i = start; i < end; ++i) {
          // demand[i] 等同于 *(demand+i)
          int next_inventory = inventory_up_level - demand[i];

          // fast clamp (branch version)
          if (next_inventory < min_I)
            next_inventory = min_I;
          else if (next_inventory > max_I)
            next_inventory = max_I;

          int next_index = next_inventory - min_I;
          double cost = fix_cost + variable_cost + H[next_index];
          current_value += prob[i] * (cost + V_next[next_index]);
        }

        if (current_value < best_value) {
          best_value = current_value;
          best_action = action;
        }
      }

      V_current[idx] = best_value; // 同时也赋值了V
      P_current[idx] = best_action;
    }
  }

  const auto end_time = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> elapsed = end_time - start_time;

  constexpr int initialInventory = 0;
  const double optimalValue = value[initialInventory - min_I];

  std::cout << "planning horizon = " << T << '\n';
  std::cout << "running time = " << elapsed.count() << " seconds\n";
  std::cout << "optimal value = " << optimalValue << '\n';
  std::cout << "optimal order at t=1, inventory=0 is: " << policy[initialInventory - min_I] << '\n';

  return 0;
}