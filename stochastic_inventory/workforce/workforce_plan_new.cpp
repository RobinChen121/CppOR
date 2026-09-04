/*
 * Created by Zhen Chen on 2026/8/31.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "workforce_plan_new.h"

#include "piecewise.h"
#include "util_binomial.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>

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
          p[j] = binomialPDF(i, ps[t], j);
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

std::pair<double, double> WorkforcePlanNew::solve_mip() const {
  // c++ 如果使用 new 创建对象，则是一个指针，访问对象时用 -> 操作符，必须使用 delete
  // 释放内存，否则会造成内存泄漏
  const auto mip = PiecewiseWorkforce(initial_workers, fix_hire_cost, unit_vari_cost, salary,
                                      unit_penalty, turnover_rates, min_workers);
  return mip.piece_approximate(piece_segment);
}

std::vector<std::array<int, 2>> WorkforcePlanNew::solve_mipsS() const {
  const auto mip = PiecewiseWorkforce(initial_workers, fix_hire_cost, unit_vari_cost, salary,
                                      unit_penalty, turnover_rates, min_workers);
  auto sS_values = mip.get_sS(piece_segment);
  return sS_values;
}

double WorkforcePlanNew::simulate_sS(const int ini_workers,
                                     const std::vector<std::array<int, 2>> &sS) const {
  std::vector<int> sample_nums(T);
  int sample_num_total = 1;
  std::vector<int> sample_num_accumulate(T);
  for (size_t t = 0; t < T; ++t) {
    if (t > 2)
      sample_nums[t] = 1;
    else
      sample_nums[t] = 30;
    sample_num_total *= sample_nums[t];
    sample_num_accumulate[t] = sample_num_total;
  }

  std::vector<std::vector<int>> inventories(T);
  std::vector<std::vector<double>> costs(T);

  std::random_device rd;  // 真随机种子（硬件）
  std::mt19937 gen(rd()); // 伪随机数引擎

  for (size_t t = 0; t < T; ++t) {
    const int N = sample_nums[t];
    const size_t last_length = t == 0 ? 1 : inventories[t - 1].size();
    inventories[t].reserve(N * last_length);
    costs[t].reserve(N * last_length);
    for (size_t i = 0; i < last_length; ++i) {
      const int this_ini_workers = t == 0 ? ini_workers : inventories[t - 1][i];

      const int Q = this_ini_workers < sS[t][0] ? sS[t][1] - this_ini_workers : 0;
      const int hire_up_to = this_ini_workers + Q;

      // C++ 标准库里有产生二项分布随机数的函数 std::binomial_distribution
      std::binomial_distribution<> dist(hire_up_to, turnover_rates[t]); // 二项分布
      for (size_t k = 0; k < N; ++k) {
        const int random_demand = dist(gen);
        const int next_workers = this_ini_workers + Q - random_demand;
        inventories[t].push_back(next_workers);
        const double fix_cost = Q > 0 ? fix_hire_cost : 0;
        const double vari_cost = unit_vari_cost * Q;
        const double salary_cost = salary * next_workers;
        const double penalty_cost =
            next_workers > min_workers[t] ? 0 : unit_penalty * (min_workers[t] - next_workers);
        const double immediate_cost = fix_cost + vari_cost + salary_cost + penalty_cost;
        if (t == 0)
          costs[t].push_back(immediate_cost);
        else
          costs[t].push_back(costs[t - 1][i] + immediate_cost);
      }
    }
  }
  const double simulate_cost =
      std::accumulate(costs[T - 1].begin(), costs[T - 1].end(), 0.0) / sample_num_total;

  std::cout << "simulate cost in " << sample_num_total << " samples is " << simulate_cost
            << std::endl;
  return simulate_cost;
}

// int main() {
//   const auto start_time = std::chrono::high_resolution_clock::now();
//   auto problem = WorkforcePlanNew();
//   auto [best_value, best_action] = problem.DP1DVector();
//
//   const auto end_time = std::chrono::high_resolution_clock::now();
//   const std::chrono::duration<double> elapsed = end_time - start_time;
//
//   std::cout << "planning horizon = " << problem.getT() << '\n';
//   std::cout << "running time of using 1D vector = " << elapsed.count() << " seconds\n";
//   // 关闭科学计数法（默认保留 6 位小数），可进一步用 << std::setprecision(n)设置小数位数
//   std::cout << std::fixed;
//   std::cout << "optimal value = " << best_value << '\n';
//   std::cout << "optimal hiring at t = 1, initial worker = " << problem.getInitialWorkers()
//             << " is: " << best_action << '\n';
//   std::cout << std::string(50, '*') << std::endl;
//
//   const auto start_time2 = std::chrono::high_resolution_clock::now();
//   auto [fst, snd] = problem.solve_mip();
//   const auto end_time2 = std::chrono::high_resolution_clock::now();
//   const std::chrono::duration<double> time2 = end_time2 - start_time2;
//   std::cout << "running time of MIP is " << time2.count() << 's' << std::endl;
//   const double gap1 = (best_value - fst) / best_value * 100;
//   std::cout << "the optimality gap by MIP is: " << std::fixed << std::setprecision(2) << gap1 <<
//   "%"
//             << std::endl;
//   const double gap2 = snd / (fst + snd) * 100;
//   std::cout << "the linearization gap by MIP is: " << std::fixed << std::setprecision(2) << gap2
//             << "%" << std::endl;
//
//   std::cout << std::string(50, '*') << std::endl;
//   const auto start_time3 = std::chrono::high_resolution_clock::now();
//   const auto sS_values = problem.solve_mipsS();
//   const auto end_time3 = std::chrono::high_resolution_clock::now();
//   const std::chrono::duration<double> time3 = end_time3 - start_time3;
//   std::cout << "running time of MIP-sS is " << time3.count() << 's' << std::endl;
//   std::cout << "s, S in each period by MIP are: " << std::endl;
//   for (const auto row : sS_values) {
//     for (const auto col : row) {
//       std::cout << col << ' ';
//     }
//     std::cout << std::endl;
//   }
//   const double mip_sS = problem.simulate_sS(problem.getInitialWorkers(), sS_values);
//   const double gap3 = (mip_sS - best_value) / best_value * 100;
//   std::cout << "the optimality gap by MIP-sS is: " << std::fixed << std::setprecision(2) << gap3
//             << "%" << std::endl;
//
//   return 0;
// }