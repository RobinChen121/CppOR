/*
 * Created by Zhen Chen on 2025/6/3.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "../../utils/PMF.h"
#include "WorkforceState.h"
#include <chrono>
#include <iostream>
#include <unordered_map>

class WorkforcePlan {
  std::vector<double> turnover_rate = {0.1, 0.1, 0.1};
  size_t T = turnover_rate.size();

  int initial_workers = 0;
  // 类初始化 {} 更安全，防止类属性窄化，例如从 double 到 int 这样的精度丢失
  WorkforceState ini_state = WorkforceState{1, initial_workers};
  double fix_hire_cost = 50.0;
  double unit_vari_cost = 0.0;
  double salary = 30.0;
  double unit_penalty = 40.0;
  std::vector<int> min_workers = std::vector<int>(T, 40);

  int max_hire_num = 500;
  int max_worker_num = 600;

  std::vector<std::vector<std::vector<double>>> pmf;
  // std::vector<std::vector<std::vector<std::array<double, 2>>>> pmf2;
  std::unordered_map<WorkforceState, double> cache_actions;
  std::unordered_map<WorkforceState, double> cache_values;

public:
  WorkforcePlan() {
    pmf = PMF::getPMFBinomial(max_worker_num, turnover_rate);
    // pmf2 = PMF::getPMFBinomial2(max_worker_num, turnover_rate);
  }

  [[nodiscard]] std::vector<int> feasible_actions() const {
    std::vector<int> actions(max_hire_num + 1);
    for (int i = 0; i <= max_hire_num; i++)
      actions[i] = i;
    return actions;
  }

  [[nodiscard]] double immediate_value(const WorkforceState ini_state, const int action,
                                       const int overturn_num) const {
    const double fix_cost = action > 0 ? fix_hire_cost : 0;
    const double vari_cost = unit_vari_cost * action;
    int next_workers = ini_state.get_initial_workers() + action - overturn_num;
    // next_workers = next_workers > max_worker_num ? max_worker_num : next_workers;
    const double salary_cost = salary * next_workers;
    const int t = ini_state.getPeriod() - 1;
    const double penalty_cost =
        next_workers > min_workers[t] ? 0 : unit_penalty * (min_workers[t] - next_workers);
    const double total_cost = fix_cost + vari_cost + salary_cost + penalty_cost;
    if (t == 2 and ini_state.get_initial_workers() == 119)
      int a = 0;
    return total_cost;
  }

  [[nodiscard]] WorkforceState state_transition(const WorkforceState ini_state, const int action,
                                                const int overturn_num) const {
    int next_workers = ini_state.get_initial_workers() + action - overturn_num;
    next_workers = next_workers > max_worker_num ? max_worker_num : next_workers;
    // 类可以直接使用列表初始化
    // 等价于 WorkforceState{ini_state.getPeriod() + 1, next_workers}
    // 等价于 WorkforceState(ini_state.getPeriod() + 1, next_workers)
    return {ini_state.getPeriod() + 1, next_workers};
  }

  double recursion(const WorkforceState ini_state) {
    int best_hire_num = 0;
    double best_cost = std::numeric_limits<double>::max();
    const int t = ini_state.getPeriod() - 1;

    for (const int action : feasible_actions()) {
      double this_value = 0;
      int turnover_num = 0;
      int hire_up_to = ini_state.get_initial_workers() + action;
      if (hire_up_to >= pmf[t].size() - 1)
        hire_up_to = static_cast<int>(pmf[t].size() - 1);

      for (const auto &d_and_p = pmf[t][hire_up_to]; const double prob : d_and_p) {
        this_value += prob * immediate_value(ini_state, action, turnover_num);
        if (t < T - 1) {
          // std::unordered_map、std::map 等容器的 .contains(key) 方法是 从 C++20 才引入的
          // 用 find 速度更快些，相对于 cache_values.at[new_state] 和 cache_values[new_state]
          // cache_values.at[new_state] 或 cache_values[new_state] 会再触发一次哈希查找
          auto new_state = state_transition(ini_state, action, turnover_num);
          if (auto it = cache_values.find(new_state); it != cache_values.end())
            this_value += prob * it->second;
          else
            this_value += prob * recursion(new_state);
        }
        ++turnover_num; // 比 turnover_num++ 更快点，因为并不使用 turnover_num 的原值
      }
      if (this_value < best_cost) {
        best_cost = this_value;
        best_hire_num = action;
      }
    }
    cache_actions[ini_state] = best_hire_num;
    cache_values[ini_state] = best_cost;
    return best_cost;
  }

  std::vector<double> solve() { return {recursion(ini_state), cache_actions.at(ini_state)}; }
};

int main() {
  auto problem = WorkforcePlan();
  const auto start_time = std::chrono::high_resolution_clock::now();
  const auto final_value = problem.solve()[0];
  const auto end_time = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> time = end_time - start_time;
  std::cout << "running time is " << time.count() << 's' << std::endl;
  std::cout << "Final optimal cost is " << final_value << std::endl;
  std::cout << "Optimal hiring number in the first period is " << problem.solve()[1] << std::endl;

  return 0;
}

// double recursion2(const WorkforceState ini_state) {
//   const auto actions = feasible_actions();
//   const int initial_workers = ini_state.get_initial_workers();
//   const int t = ini_state.getPeriod() - 1;
//
//   int best_hire_num = 0;
//   std::vector<double> policy_values(feasible_actions().size());
//   double best_cost = std::numeric_limits<double>::max();
//
//   for (size_t i = 0; i < actions.size(); i++) {
//     const int hire_num = actions[i];
//     int hire_up_to = initial_workers + hire_num;
//     if (hire_up_to >= pmf2[t].size() - 1)
//       hire_up_to = static_cast<int>(pmf2[t].size() - 1);
//     const auto &d_and_p = pmf2[t][hire_up_to];
//     double this_value = 0;
//
//     for (size_t j = 0; j < d_and_p.size(); j++) {
//       const int demand = static_cast<int>(d_and_p[j][0]);
//       double temp_value = immediate_value(ini_state, hire_num, demand);
//       this_value += d_and_p[j][1] * temp_value;
//       if (t < T - 1) {
//         // std::unordered_map、std::map 等容器的 .contains(key) 方法是 从 C++20 才引入的
//         // 用 find 速度更快些，相对于 cache_values.at[new_state] 和 cache_values[new_state]
//         // cache_values.at[new_state] 或 cache_values[new_state] 会再触发一次哈希查找
//         auto new_state = state_transition(ini_state, hire_num, demand);
//         // if (cache_values.contains(new_state))
//         //   this_value += d_and_p[j] * cache_values.at(new_state);
//         if (auto it = cache_values.find(new_state); it != cache_values.end()) {
//           this_value += d_and_p[j][1] * it->second;
//         } else {
//           double temp_value2 = recursion2(new_state);
//           this_value += d_and_p[j][1] * temp_value2;
//         }
//       }
//     }
//     if (t == 2 and hire_up_to >= pmf2[t].size() - 1)
//       int a = 0;
//     policy_values[i] = this_value;
//     if (policy_values[i] < best_cost) {
//       best_cost = policy_values[i];
//       best_hire_num = hire_num;
//     }
//   }
//   cache_values[ini_state] = best_cost;
//   cache_actions[ini_state] = best_hire_num;
//   return best_cost;
// }