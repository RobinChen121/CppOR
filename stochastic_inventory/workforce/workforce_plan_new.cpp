/*
 * Created by Zhen Chen on 2026/8/31.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "workforce_plan_new.h"

#include <cmath>
#include <iostream>
#include <limits>

// Binomial PMF: P(X = k)
double binomialPdf(const int n, const int k, const double p) {
  if (k < 0 || k > n)
    return 0.0;
  if (p < 0.0 || p > 1.0)
    return std::numeric_limits<double>::quiet_NaN();

  if (p == 0.0)
    return (k == 0) ? 1.0 : 0.0;
  if (p == 1.0)
    return (k == n) ? 1.0 : 0.0;

  // lgamma(k+1) gives log(k!)
  // 通过计算log，避免较大的阶乘溢出
  const double log_prob = std::lgamma(n + 1.0) - std::lgamma(k + 1.0) - std::lgamma(n - k + 1.0) +
                          k * std::log(p) + (n - k) * std::log(1.0 - p);

  return std::exp(log_prob);
}

PMFData getPMFBinomial(const int max_staff, const std::vector<double> &ps) {
  const size_t T = ps.size();
  PMFData result;

  result.offset.resize(max_staff + 1);
  for (int i = 0; i <= max_staff; ++i) {
    result.offset[i] = static_cast<size_t>(i) * (i + 1) / 2;
  }

  result.per_t = static_cast<size_t>(max_staff + 1) * static_cast<size_t>(max_staff + 2) / 2;
  result.prob.resize(T * result.per_t);

  for (size_t t = 0; t < T; ++t) {
    double *pmf_t = result.prob.data() + t * result.per_t; // 每个t的起始位置，是一个指针
    for (int i = 0; i <= max_staff; ++i) {
      double *p = pmf_t + result.offset[i]; // 阶段t时，每个 i 的起始位置，是一个指针
      if (i == 0) {
        p[0] = 1.0;
      } else {
        for (int j = 0; j <= i; ++j) {
          p[j] = binomialPdf(i, j, ps[t]);
        }
      }
    }
  }
  return result;
}

// precompute the sum of salary and penalty costs for each possible staff level index
std::vector<double> WorkforcePlanNew::computeSalaryPenaltyCost(const int t) const {
  std::vector<double> salary_penalty_costs(state_num);
  for (int i = 0; i < state_num; ++i) {
    const int x = i;
    const double salary_cost = salary * x;
    salary_penalty_costs[i] =
        (x - min_workers[t] > 0) ? salary_cost : salary_cost - unit_penalty * (x - min_workers[t]);
  }
  return salary_penalty_costs;
}

// precompute expected cost for each order-up-to level y
// expected cost including current holding and penalty costs and future costs
std::vector<double> WorkforcePlanNew::computeExpectCost(const int t) const {
  // base index for the next value and policy arrays
  const int base_next = (t + 1) * state_num;

  const auto salary_penalty_costs = computeSalaryPenaltyCost(t);

  // start index in the pmf for this t
  const size_t start = static_cast<size_t>(t) * pmf.per_t;

  const int y_max = max_worker_num;
  const int y_len = y_max + 1;
  std::vector expected_cost(y_len, 0.0);

  for (int y = 0; y <= y_max; ++y) {
    double y_cost = 0.0;
    const double *p = pmf.prob.data() + start +
                      pmf.offset[y]; // pointer to the start of the probabilities for this y
    for (int i = 0; i <= y; ++i) {
      const int next_inventory = y - i;
      const int next_index = next_inventory;
      y_cost += p[i] * (salary_penalty_costs[next_index] + value[base_next + next_index]);
    }
    expected_cost[y] = y_cost;
  }
  return expected_cost;
}

// DP using 1D vector for value and policy arrays
std::pair<double, double> WorkforcePlanNew::DP1DVector() {

  for (int t = T - 1; t >= 0; --t) {
    // base index for the current value and policy arrays
    const int base_current = t * state_num;

    // precompute expected cost for each order-up-to level y
    const auto expected_cost = computeExpectCost(t);

    for (int idx = 0; idx < state_num; ++idx) {
      const int inventory = idx;
      double best_value = INF;
      int best_action = 0;

      for (int action = 0; action <= max_hire_num; ++action) {
        const double fix_cost = (action > 0) ? fix_hire_cost : 0.0;
        const double variable_cost = action * unit_vari_cost;
        int inventory_up_level = inventory + action;
        if (inventory_up_level > max_worker_num)
          inventory_up_level = max_worker_num;

        const double current_value = (fix_cost + variable_cost) + expected_cost[inventory_up_level];

        if (current_value < best_value) {
          best_value = current_value;
          best_action = action;
        }
      }

      value[base_current + idx] = best_value;
      policy[base_current + idx] = best_action;
    }
  }

  return {value[static_cast<int>(initial_workers)], policy[static_cast<int>(initial_workers)]};
}

int main() {
  const auto start_time = std::chrono::high_resolution_clock::now();
  auto problem = WorkforcePlanNew();
  auto [best_value, best_action] = problem.DP1DVector();

  const auto end_time = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> elapsed = end_time - start_time;

  std::cout << "planning horizon = " << problem.getT() << '\n';
  std::cout << "running time of using 1D vector = " << elapsed.count() << " seconds\n";
  // 关闭科学计数法（默认保留 6 位小数），可进一步用 << std::setprecision(n)设置小数位数
  std::cout << std::fixed;
  std::cout << "optimal value = " << best_value << '\n';
  std::cout << "optimal hiring at t = 1, initial worker = " << problem.getInitialWorkers()
            << " is: " << best_action << '\n';
  std::cout << "*******************************" << std::endl;

  return 0;
}