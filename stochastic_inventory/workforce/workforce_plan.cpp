/*
 * Created by Zhen Chen on 2025/6/3.
 * Email: chen.zhen5526@gmail.com
 * Description: For a 3 period problem, c++ serial can solve in 0.63 seconds while java is 7s.
 * For 5 periods, c++ parallel 8 threads mac m1 time is 0.77s while serial is 1.76s.
 * dell 7740, 12 threads, time is 1.85s while serial is 3.98s
 *
 */

#include "../../utils/PMF.h"
#include "WorkerState.h"
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_map>

enum class Direction { FORWARD, BACKWARD };
enum class ComputeGy { True, False };

class WorkforcePlan {
  Direction direction = Direction::BACKWARD;

  std::vector<double> turnover_rate = {0.3, 0.1, 0.2};
  size_t T = turnover_rate.size();

  int initial_workers = 0;
  // 类初始化 {} 更安全，防止类属性窄化，例如从 double 到 int 这样的精度丢失
  WorkerState ini_state = WorkerState{1, initial_workers};
  double fix_hire_cost = 50.0;
  double unit_vari_cost = 0.0;
  double salary = 30.0;
  double unit_penalty = 40.0;
  std::vector<int> min_workers = std::vector<int>(T, 40);

  int max_hire_num = 500;
  int max_worker_num = 600;

  std::vector<std::vector<std::vector<double>>> pmf;
  // std::vector<std::vector<std::vector<std::array<double, 2>>>> pmf2;
  std::unordered_map<WorkerState, double> cache_actions;
  std::unordered_map<WorkerState, double> cache_values;

  std::mutex mtx; // 互斥锁保护共享数据写入

public:
  std::vector<std::unordered_map<WorkerState, double>> value;
  std::vector<std::unordered_map<WorkerState, double>> policy;

  WorkforcePlan() {
    pmf = PMF::getPMFBinomial(max_worker_num, turnover_rate);
    // pmf2 = PMF::getPMFBinomial2(max_worker_num, turnover_rate);
  }

  Direction get_direction() const { return direction; };
  WorkerState get_initial_state() const { return ini_state; };
  [[nodiscard]] std::vector<int> feasibleActions() const {
    std::vector<int> actions(max_hire_num + 1);
    for (int i = 0; i <= max_hire_num; i++)
      actions[i] = i;
    return actions;
  }

  [[nodiscard]] double immediateValue(const WorkerState ini_state, const int action,
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

  [[nodiscard]] WorkerState stateTransition(const WorkerState ini_state, const int action,
                                            const int overturn_num) const {
    int next_workers = ini_state.getInitialWorkers() + action - overturn_num;
    next_workers = next_workers > max_worker_num ? max_worker_num : next_workers;
    // 类可以直接使用列表初始化
    // 等价于 WorkerState{ini_state.getPeriod() + 1, next_workers}
    // 等价于 WorkerState(ini_state.getPeriod() + 1, next_workers)
    return {ini_state.getPeriod() + 1, next_workers};
  }

  double recursionForward(const WorkerState ini_state) {
    int best_hire_num = 0;
    double best_cost = std::numeric_limits<double>::max();
    const int t = ini_state.getPeriod() - 1;

    for (const int action : feasibleActions()) {
      double this_value = 0;
      int turnover_num = 0;
      int hire_up_to = ini_state.getInitialWorkers() + action;
      if (hire_up_to >= pmf[t].size() - 1)
        hire_up_to = static_cast<int>(pmf[t].size() - 1);

      for (const auto &d_and_p = pmf[t][hire_up_to]; const double prob : d_and_p) {
        this_value += prob * immediateValue(ini_state, action, turnover_num);
        if (t < T - 1) {
          // std::unordered_map、std::map 等容器的 .contains(key) 方法是 从 C++20 才引入的
          // 用 find 速度更快些，相对于 cache_values.at[new_state] 和 cache_values[new_state]
          // cache_values.at[new_state] 或 cache_values[new_state] 会再触发一次哈希查找
          auto new_state = stateTransition(ini_state, action, turnover_num);
          if (auto it = cache_values.find(new_state); it != cache_values.end())
            this_value += prob * it->second;
          else
            this_value += prob * recursionForward(new_state);
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

  void recursionBackwardParallel() {
    value.resize(T + 1);
    policy.resize(T);

    // 最后一阶段边界条件
    for (int i = 0; i <= max_worker_num; ++i) {
      auto state = WorkerState(T + 1, i);
      value[T][state] = 0.0;
    }

    for (int t = static_cast<int>(T) - 1; t >= 0; --t) {
      std::vector<std::thread> threads;
      const int thread_num = static_cast<int>(std::thread::hardware_concurrency());
      const int chunk_size = max_worker_num / thread_num;
      for (int thread_index = 0; thread_index < thread_num; ++thread_index) {
        const int start_workers = thread_index * chunk_size;
        const int end_workers =
            (thread_index == thread_num - 1) ? max_worker_num : start_workers + chunk_size;

        // emplace_back 是 std::vector 提供的一个成员函数，用于在向量末尾直接构造一个新对象
        // 而不是先创建对象再插入（相比 push_back 更高效）

        // 语法为：emplace_back(func, arg)
        // 但若 func 为非静态成员函数，需要用到 this，并且前面的成员函数要用引用
        // std::ref(x) 会把 x 包装成一个可以被复制但实际是引用的对象
        // 但是原函数构造中不用 & 在大规模数据时会影响计算速度

        // std::thread 构造函数参数始终默认按值传递, 即使参数前用 & 也是，此时 & 表示指针，不是引用
        // & 在声明变量或函数构造时中跟参数表示引用，调用函数时跟参数表示指针
        threads.emplace_back(&WorkforcePlan::computeStage, this, t, start_workers, end_workers,
                             std::ref(value), std::ref(policy));
      }
      for (auto &thread : threads) {
        // 不要忘了关闭线程
        thread.join();
      }
    }
  }

  void computeStage(const int t, const int start_inventory, const int end_inventory,
                    std::vector<std::unordered_map<WorkerState, double>> &value,
                    std::vector<std::unordered_map<WorkerState, double>> &policy) {
    for (int i = start_inventory; i <= end_inventory; i++) {
      double bestQ = 0.0;
      double best_value = std::numeric_limits<double>::max();
      WorkerState ini_state(t + 1, i);
      for (const std::vector<int> actions = feasibleActions(); const int action : actions) {
        double this_value = 0;
        int demand = 0;
        int hire_up_to = ini_state.getInitialWorkers() + action;
        if (hire_up_to >= pmf[t].size() - 1)
          hire_up_to = static_cast<int>(pmf[t].size() - 1);
        for (const auto &demand_prob : pmf[t][hire_up_to]) {
          this_value += demand_prob * immediateValue(ini_state, action, demand);
          if (t < T - 1) {
            auto new_state = stateTransition(ini_state, action, demand);
            this_value += demand_prob * value[t + 1][new_state];
          }
          ++demand;
        }
        if (this_value < best_value) {
          best_value = this_value;
          bestQ = action;
        }
      }

      std::lock_guard<std::mutex> lock(mtx); // 保护共享数据写入
      value[t][ini_state] = best_value;
      policy[t][ini_state] = bestQ;
    }
  }

  std::vector<double> solve() {
    if (direction == Direction::FORWARD)
      return {recursionForward(ini_state), cache_actions.at(ini_state)};
    recursionBackwardParallel();
    return {value[0][ini_state], policy[0][ini_state]};
  }

  std::vector<std::array<int, 2>> findsS() const {
    std::vector<std::array<int, 2>> arr(T);

    if (direction == Direction::FORWARD) {
      std::map<WorkerState, int> ordered_cache_actions(cache_actions.begin(), cache_actions.end());
      int t_index = 1;
      for (const auto &[fst, snd] : ordered_cache_actions) {
        if (fst.getPeriod() == t_index) {
          if (snd != 0)
            arr[t_index - 1][1] = fst.getInitialWorkers() + snd;
          else {
            arr[t_index - 1][0] = fst.getInitialWorkers();
            ++t_index;
            break;
          }
        }
      }
      return arr;
    }

    for (size_t t = 0; t < policy.size(); ++t) {
      for (std::map<WorkerState, int> ordered_cache_actions(policy[t].begin(), policy[t].end());
           const auto &[fst, snd] : ordered_cache_actions) {
        if (fst.getPeriod() == t + 1) {
          if (snd != 0)
            arr[t][1] = fst.getInitialWorkers() + snd;
          else {
            arr[t][0] = fst.getInitialWorkers();
            break;
          }
        }
      }
    }
    return arr;
  }

  void simulatesS(const WorkerState ini_state, const std::vector<std::array<int, 2>> &sS) const {
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
      const int K = sample_nums[t];
      const size_t last_length = t == 0 ? 1 : inventories[t - 1].size();
      inventories[t].reserve(K * last_length);
      costs[t].reserve(K * last_length);
      for (size_t i = 0; i < last_length; ++i) {
        WorkerState this_ini_state =
            t == 0 ? ini_state : WorkerState{static_cast<int>(t) + 1, inventories[t - 1][i]};

        const int ini_workers = this_ini_state.getInitialWorkers();
        const int Q = ini_workers < sS[t][0] ? sS[t][1] - ini_workers : 0;
        const int hire_up_to = ini_workers + Q;

        std::binomial_distribution<> dist(hire_up_to, turnover_rate[t]); // 二项分布
        for (size_t k = 0; k < K; ++k) {
          const int random_demand = dist(gen);
          WorkerState new_state = stateTransition(this_ini_state, Q, random_demand);
          inventories[t].push_back(new_state.getInitialWorkers());
          if (t == 0)
            costs[t].push_back(immediateValue(this_ini_state, Q, random_demand));
          else
            costs[t].push_back(costs[t - 1][i] + immediateValue(this_ini_state, Q, random_demand));
        }
      }
    }
    const double simulate_cost =
        std::accumulate(costs[T - 1].begin(), costs[T - 1].end(), 0.0) / sample_num_total;

    std::cout << "simulate cost in " << sample_num_total << " samples is " << simulate_cost
              << std::endl;
  }

  void checkKConvexity() {}

  std::vector<std::vector<double>> getOptTable() const {
    std::vector<std::vector<double>> arr;
    if (direction == Direction::FORWARD) {
      arr.reserve(cache_actions.size()); // 预分配空间

      // map 可以通过迭代范围构造
      std::map<WorkerState, double> ordered_cache_actions(cache_actions.begin(),
                                                          cache_actions.end());

      for (const auto &[fst, snd] : ordered_cache_actions) {
        arr.push_back({static_cast<double>(fst.getPeriod()),
                       static_cast<double>(fst.getInitialWorkers()), snd});
      }
      return arr;
    }
    arr.reserve(policy.size() * policy[0].size());
    for (const auto &t : policy) {
      for (std::map<WorkerState, double> ordered_cache_actions(t.begin(), t.end());
           const auto &[fst, snd] : ordered_cache_actions) {
        arr.push_back({static_cast<double>(fst.getPeriod()),
                       static_cast<double>(fst.getInitialWorkers()), snd});
      }
    }
    return arr;
  }
};

int main() {
  auto problem = WorkforcePlan();
  const auto start_time = std::chrono::high_resolution_clock::now();
  const auto final_value = problem.solve()[0];
  const auto end_time = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> time = end_time - start_time;
  if (problem.get_direction() == Direction::FORWARD)
    std::cout << "running time is " << time.count() << 's' << std::endl;
  else {
    const int thread_num = static_cast<int>(std::thread::hardware_concurrency());
    std::cout << "running time of C++ in parallel with " << thread_num << " threads is "
              << time.count() << 's' << std::endl;
  }
  std::cout << "Final optimal cost is " << final_value << std::endl;
  std::cout << "Optimal hiring number in the first period is " << problem.solve()[1] << std::endl;

  const auto arr_sS = problem.findsS();
  std::cout << "s, S in each period are: " << std::endl;
  for (const auto row : arr_sS) {
    for (const auto col : row) {
      std::cout << col << ' ';
    }
    std::cout << std::endl;
  }

  problem.simulatesS(problem.get_initial_state(), arr_sS);

  return 0;
}

// double recursion2(const WorkerState ini_state) {
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