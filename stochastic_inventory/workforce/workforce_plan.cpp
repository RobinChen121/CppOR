/*
 * Created by Zhen Chen on 2025/6/3.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "../../utils/PMF.h"
#include "WorkforceState.h"
#include <iostream>

class WorkforcePlan {
  std::vector<double> turnover_rate = {0.5};
  size_t T = turnover_rate.size();

  int initial_workers = 0;
  // 类初始化 {} 更安全，防止类属性窄化，例如从 double 到 int 这样的精度丢失
  WorkforceState ini_state = WorkforceState{1, initial_workers};
  double fix_hire_cost = 50.0;
  double unit_vari_cost = 0.0;
  double salary = 30.0;
  double unit_penalty = 100.0;
  std::vector<int> min_workers = std::vector<int>(T, 40);

  int max_hire_num = 500;
  int max_worker_num = 600;

  std::vector<std::vector<std::vector<double>>> pmf;
  std::unordered_map<WorkforceState, double> cache_actions;
  std::unordered_map<WorkforceState, double> cache_values;

public:
  WorkforcePlan() { pmf = PMF::getPMFBinomial(max_worker_num, turnover_rate); }

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
    int next_workers = ini_state.getInitialWorkers() + action - overturn_num;
    next_workers = next_workers > max_worker_num ? max_worker_num : next_workers;
    const double salary_cost = salary * next_workers;
    const int t = ini_state.getPeriod() - 1;
    const double penalty_cost =
        next_workers > min_workers[t] ? 0 : unit_penalty * (min_workers[t] - next_workers);
    const double total_cost = fix_cost + vari_cost + salary_cost + penalty_cost;
    return total_cost;
  }

  [[nodiscard]] WorkforceState state_transition(const WorkforceState ini_state, const int action,
                                                const int overturn_num) const {
    int next_workers = ini_state.getInitialWorkers() + action - overturn_num;
    next_workers = next_workers > max_worker_num ? max_worker_num : next_workers;
    // 类可以直接使用列表初始化
    // 等价于 WorkforceState{ini_state.getPeriod() + 1, next_workers}
    // 等价于 WorkforceState(ini_state.getPeriod() + 1, next_workers)
    return {ini_state.getPeriod() + 1, next_workers};
  }

  double recursion(const WorkforceState ini_state) {
    int best_hire_num = 0;
    double best_cost = std::numeric_limits<double>::max();
    for (const int action : feasible_actions()) {
      double this_value = 0;
      const int t_index = ini_state.getPeriod() - 1;
      const auto &d_and_p = pmf[t_index];
      int turnover_num = 0;
      int hire_up_to = ini_state.getInitialWorkers() + action;
      if (hire_up_to >= d_and_p.size() - 1)
        hire_up_to = static_cast<int>(d_and_p.size()) - 1;
      try {
        for (const double prob : d_and_p[hire_up_to]) {
          this_value += prob * immediate_value(ini_state, action, turnover_num);
          if (t_index < T - 1) {
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
      } catch (...) { // ... 表示所有异常
        int temp1 = ini_state.getInitialWorkers();
        int temp2 = action;
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