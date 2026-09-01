/*
 * Created by Zhen Chen on 2026/9/1.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "newsvendor_general.h"

#include <iostream>
#include <limits>

// get the probability mass function value of Poisson
double poissonPmf(const int k, const int lambda) {
  if (k < 0 || lambda < 0)
    return 0.0;
  if (lambda == 0)
    return k == 0 ? 1.0 : 0.0;

  // lgamma gives log(k!)
  const double logP = -lambda + static_cast<double>(k) * std::log(static_cast<double>(lambda)) -
                      std::lgamma(static_cast<double>(k) + 1.0);
  return std::exp(logP);
}

double poissonCdf(const int k, const int lambda) {
  if (k < 0)
    return 0.0;
  if (lambda < 0)
    return 0.0;
  if (lambda == 0)
    return 1.0;

  double cumulative = 0.0;
  double term = std::exp(-static_cast<double>(lambda)); // P(X=0)
  for (int i = 0; i <= k; ++i) {
    cumulative += term;
    if (i < k)
      term *= static_cast<double>(lambda) / static_cast<double>(i + 1); // 递推计算 P(X=i)
  }
  return cumulative;
}

int poissonQuantile(const double p, const int lambda) {
  if (p <= 0.0)
    return 0;
  if (p >= 1.0)
    return std::numeric_limits<int>::max();
  if (lambda < 0)
    return 0;

  int low = 0;
  int high = lambda;
  // 动态上界，不是 boost 库中的具体实现，避免在 lambda 很大时计算量过大
  while (poissonCdf(high, lambda) < p) {
    if (high > std::numeric_limits<int>::max() / 2)
      return std::numeric_limits<int>::max();
    high *= 2;
  }

  while (low < high) {
    if (const int mid = (low + high) / 2; poissonCdf(mid, lambda) < p) {
      low = mid + 1;
    } else {
      high = mid;
    }
  }
  return low;
}

PMFData getPMFPoisson(const std::vector<int> &demands, const double truncated_quantile) {
  const size_t T = demands.size();

  std::vector<int> support_lb(T);
  std::vector<int> support_ub(T);

  // compute bounds
  for (size_t t = 0; t < T; ++t) {
    support_lb[t] = poissonQuantile(1 - truncated_quantile, demands[t]);
    support_ub[t] = poissonQuantile(truncated_quantile, demands[t]);
  }

  PMFData pmf;

  pmf.start_index_t.resize(T + 1, 0);
  pmf.len_t.resize(T);

  // compute total size (for allocation)
  int total_size = 0;
  for (size_t t = 0; t < T; ++t) {
    pmf.len_t[t] = support_ub[t] - support_lb[t] + 1;
    pmf.start_index_t[t] = total_size;
    total_size += pmf.len_t[t];
  }
  pmf.start_index_t[T] = total_size;

  pmf.demands.resize(total_size);
  pmf.prob.resize(total_size);

  // fill flattened arrays
  for (size_t t = 0; t < T; ++t) {
    const int base = pmf.start_index_t[t];
    const int len_t = pmf.len_t[t];
    const int lb = support_lb[t];

    const double norm = 1.0 / (2.0 * truncated_quantile - 1.0);

    for (int j = 0; j < len_t; ++j) {
      const int demand = lb + j;
      const int idx = base + j;

      pmf.demands[idx] = demand;
      pmf.prob[idx] = poissonPmf(demand, demands[t]) * norm;
    }
  }

  return pmf;
}

std::vector<std::vector<std::array<double, 2>>> getPMFPoisson2(const std::vector<int> &demands,
                                                               const double truncated_quantile) {
  const size_t T = demands.size();
  std::vector<int> support_lb(T);
  std::vector<int> support_ub(T);
  for (size_t i = 0; i < T; ++i) {
    support_ub[i] = poissonQuantile(truncated_quantile, demands[i]);
    support_lb[i] = poissonQuantile(1 - truncated_quantile, demands[i]);
  }
  std::vector pmf(T, std::vector<std::array<double, 2>>());
  for (int t = 0; t < T; ++t) {
    const int demand_length = static_cast<int>((support_ub[t] - support_lb[t] + 1));
    pmf[t].resize(demand_length, std::array<double, 2>());
    for (int j = 0; j < demand_length; ++j) {
      pmf[t][j][0] = support_lb[t] + j;
      const int demand = static_cast<int>(pmf[t][j][0]);
      pmf[t][j][1] =
          poissonPmf(demand, static_cast<int>(demands[t])) / (2 * truncated_quantile - 1);
    }
  }
  return pmf;
}

// precompute the sum of holding and penalty costs for each possible inventory level index
std::vector<double> Newsvendor::computeHoldPenaltyCost() const {
  std::vector<double> hold_penalty_costs(num_inv);
  for (int i = 0; i < num_inv; ++i) {
    const int x = i + min_I;
    hold_penalty_costs[i] = (x > 0) ? hold_cost * x : -penalty_cost * x;
  }
  return hold_penalty_costs;
}

// precompute expected cost for each order-up-to level y
// expected cost including current holding and penalty costs and future costs
std::vector<double> Newsvendor::computeExpectCost(const int t) const {
  // base index for the next value and policy arrays
  const int base_next = (t + 1) * num_inv;

  const auto hold_penalty_costs = computeHoldPenaltyCost();

  // start and end indices in the pmf
  const int start = pmf.start_index_t[t];
  const int end = pmf.start_index_t[t + 1];

  const int y_min = min_I;
  const int y_max = max_I + capacity;
  const int y_len = y_max - y_min + 1;
  std::vector expected_cost(y_len, 0.0);

  for (int y = y_min; y <= y_max; ++y) {
    double y_cost = 0.0;
    for (int i = start; i < end; ++i) {
      int next_inventory = y - pmf.demands[i];
      if (next_inventory < min_I)
        next_inventory = min_I;
      else if (next_inventory > max_I)
        next_inventory = max_I;
      const int next_index = next_inventory - min_I;
      y_cost += pmf.prob[i] * (hold_penalty_costs[next_index] + value[base_next + next_index]);
    }
    expected_cost[y - y_min] = y_cost;
  }
  return expected_cost;
}

// DP using 1D vector for value and policy arrays
std::pair<double, double> Newsvendor::DP1DVector() {

  for (int t = T - 1; t >= 0; --t) {
    // base index for the current value and policy arrays
    const int base_current = t * num_inv;

    // precompute expected cost for each order-up-to level y
    const auto expected_cost = computeExpectCost(t);

    for (int idx = 0; idx < num_inv; ++idx) {
      const int inventory = idx + min_I;
      double best_value = INF;
      int best_action = 0;

      for (int action = 0; action <= capacity; ++action) {
        const double fix_cost = (action > 0) ? fix_order_cost : 0.0;
        const double variable_cost = action * unit_order_cost;
        const int inventory_up_level = inventory + action;

        const double current_value =
            (fix_cost + variable_cost) + expected_cost[inventory_up_level - min_I];

        if (current_value < best_value) {
          best_value = current_value;
          best_action = action;
        }
      }

      value[base_current + idx] = best_value; // 同时也赋值了V
      policy[base_current + idx] = best_action;
    }
  }

  return {value[static_cast<int>(ini_inventory) - min_I],
          policy[static_cast<int>(ini_inventory) - min_I]};
}

std::vector<int> Newsvendor::getFeasibleActions() {
  for (int i = 0; i <= capacity; ++i) {
    feasible_actions[i] = i;
  }
  return feasible_actions;
}

double Newsvendor::recursion(const State &state) {
  double bestQ = 0.0;
  double bestValue = std::numeric_limits<double>::max();
  for (const int action : feasible_actions) {
    // 将这两个确定的成本计算提前，避免因为下面期望成本计算导致的精度损失，因为概率和不一定严格等于1
    const double fixCost = action > 0 ? fix_order_cost : 0.0;
    const double variableCost = action * unit_order_cost;
    double thisValue = fixCost + variableCost;

    for (const auto &demandAndProb : pmf2[state.getPeriod() - 1]) {

      const double demand = demandAndProb[0];
      const double prob = demandAndProb[1];

      const double nextInventory =
          std::clamp(state.getIniInventory() + action - demand, static_cast<double>(min_I),
                     static_cast<double>(max_I));

      const double holdingPenalty =
          nextInventory > 0 ? hold_cost * nextInventory : -penalty_cost * nextInventory;

      double futureValue = 0.0;
      if (state.getPeriod() < T) {
        const State newState{state.getPeriod() + 1, nextInventory};
        auto it = cache_values.find(newState);
        if (it != cache_values.end()) {
          futureValue = it->second;
        } else {
          futureValue = recursion(newState);
        }
      }
      thisValue += prob * (holdingPenalty + futureValue);
    }

    if (thisValue < bestValue) {
      bestValue = thisValue;
      bestQ = action;
    }
  }

  cache_actions[state] = bestQ;
  cache_values[state] = bestValue;
  return bestValue;
}

int main() {
  const auto start_time = std::chrono::high_resolution_clock::now();
  auto problem = Newsvendor();
  auto [fst, snd] = problem.DP1DVector();

  const auto end_time = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> elapsed = end_time - start_time;

  std::cout << "planning horizon = " << problem.getT() << '\n';
  std::cout << "running time of using 1D vector = " << elapsed.count() << " seconds\n";
  std::cout << "optimal value = " << fst << '\n';
  std::cout << "optimal order at t = 1, inventory = " << problem.getIniInventory() << " is: " << snd
            << '\n';
  std::cout << "*******************************" << std::endl;

  auto problem2 = Newsvendor(State(1, 0));
  const double value = problem2.recursion(State(1, 0));
  const auto end_time2 = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> elapsed2 = end_time2 - end_time;
  std::cout << "running time of using map = " << elapsed2.count() << " seconds\n";
  std::cout << "optimal value = " << value << '\n';
  std::cout << "optimal order at t = 1, inventory = " << problem2.getIniInventory()
            << " is: " << problem2.getOptQ() << '\n';
  return 0;
}