/*
 * Created by Zhen Chen on 2025/3/9.
 * Email: chen.zhen5526@gmail.com
 * Description:
 * planning horizon is 40 periods, capacity 150
 * running time of C++ in serial is 4.57606s(mac)
 * Final optimal value is: 1359.28
 * running time of C++ in parallel is 1.01151s (8 threads, mac).
 *
 * planning horizon 70 periods, capacity 100, parallel computing time is 1.3s (mac), 1.9s(dell),
 * 3.47s(dell), 2.44s(mac) for normal distribution.
 * serial is 9.47s (dell).
 *
 * when finding s, S, parallel computing is not encouraged because of truncated state.
 *
 * a good example for draw k convex of poisson demands:
 * double fixedOrderingCost = 500; double
 * variOrderingCost = 0; double penaltyCost = 10; double[]
 * meanDemand = {9, 23, 53, 29}; double holdingCost = 2; int
 * C = 100;
 * when C = 50, it is not K-convex;
 * when C = 60, it is a counter example of the optimality of multi-level (s, S) policy:
 * when initial inventory falls between s1 and s2, it is optimal to not order.
 *
 */

#include "newsvendor.h"
#include "../../utils/Kconvexity.h"
#include "../../utils/draw_graph.h"
#include "../../utils/pmf.h"
#include <algorithm> // std::max
#include <array>
#include <chrono>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <thread>

std::vector<double> NewsvendorDP::feasibleActions() const {
  const int QNum = static_cast<int>(capacity / stepSize);
  std::vector<double> actions(QNum + 1);
  for (int i = 0; i <= QNum; i = i + 1) {
    actions[i] = i * stepSize;
  }
  return actions;
}

State NewsvendorDP::stateTransitionFunction(const State &state, const double action,
                                            const double demand) const {
  double next_inventory = state.getInitialInventory() + action - demand;
  if (compute_Gy == true and state.getPeriod() == 1)
    next_inventory = state.getInitialInventory() - demand;
  if (!compute_Gy and parallel) {
    next_inventory = next_inventory > max_I ? max_I : next_inventory;
    next_inventory = next_inventory < min_I ? min_I : next_inventory;
  }

  const int nextPeriod = state.getPeriod() + 1;
  // C++11 引入了统一的列表初始化（Uniform Initialization），鼓励使用大括号 {} 初始化类
  const auto newState = State{nextPeriod, next_inventory};

  return newState;
}

double NewsvendorDP::immediateValueFunction(const State &state, const double action,
                                            const double demand) const {
  double fix_cost = action > 0 ? fix_order_cost : 0;
  double vari_cost = action * unit_vari_order_cost;
  double next_inventory = state.getInitialInventory() + action - demand;
  if (compute_Gy == true and state.getPeriod() == 1) {
    fix_cost = 0;
    vari_cost = unit_vari_order_cost * state.getInitialInventory();
    next_inventory = state.getInitialInventory() - demand;
  }
  // it is better to not truncate the state if looking for the values of s and S
  if (!compute_Gy and parallel) {
    next_inventory = next_inventory > max_I ? max_I : next_inventory;
    next_inventory = next_inventory < min_I ? min_I : next_inventory;
  }
  const double hold_cost = std::max(unit_hold_cost * next_inventory, 0.0);
  const double penalty_cost = std::max(-unit_penalty_cost * next_inventory, 0.0);

  const double total_cost = fix_cost + vari_cost + hold_cost + penalty_cost;
  return total_cost;
}

auto NewsvendorDP::getTable() const {
  std::map ordered_cache_actions(cache_actions.begin(), cache_actions.end());
  const size_t stateNums = cache_actions.size();
  std::vector table(stateNums, std::vector<double>(3));
  int index = 0;
  for (const auto &[fst, snd] : ordered_cache_actions) {
    table[index][0] = fst.getPeriod();
    table[index][1] = fst.getInitialInventory();
    table[index][2] = snd;
    index++;
  }
  return table;
}

double NewsvendorDP::getOptAction(const State &state) { return cache_actions[state]; }

// NOLINTNEXTLINE(misc-no-recursion)
double NewsvendorDP::recursion_serial(const State &state) {
  double bestQ = 0.0;
  double bestValue = std::numeric_limits<double>::max();
  const std::vector<double> actions = feasibleActions();
  for (const double action : feasibleActions()) {
    double thisValue = 0;
    for (auto demandAndProb : pmf[state.getPeriod() - 1]) {
      thisValue += demandAndProb[1] * immediateValueFunction(state, action, demandAndProb[0]);
      if (state.getPeriod() < T) {
        auto newState = stateTransitionFunction(state, action, demandAndProb[0]);
        if (cache_values.contains(newState)) {
          // some issues here
          thisValue += demandAndProb[1] * cache_values[newState];
        } else {
          thisValue += demandAndProb[1] * recursion_serial(newState);
        }
      }
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

// 不要加 const, 否则无法加锁
// 在 const 成员函数中，所有的成员变量都被认为是只读的。
// 而 std::mutex 的 lock() 会修改 mutex，所以 编译器拒绝你修改它。
void NewsvendorDP::computeStage(const int t, const int start_inventory, const int end_inventory) {
  for (int i = start_inventory; i <= end_inventory; i++) {
    double bestQ = 0.0;
    double bestValue = std::numeric_limits<double>::max();
    State state(t + 1, i);
    const std::vector<double> actions = feasibleActions();
    for (const double action : actions) {
      double thisValue = 0;
      for (auto demandAndProb : pmf[t]) {
        thisValue += demandAndProb[1] * immediateValueFunction(state, action, demandAndProb[0]);
        if (t < T - 1) {
          auto newState = stateTransitionFunction(state, action, demandAndProb[0]);
          // if there is no key, the map  will build the key and set the map value to
          // be 0.0
          if (value[t + 1].contains(newState))
            thisValue += demandAndProb[1] * value[t + 1][newState];
          else {
            // some issues here
            thisValue += demandAndProb[1] * value[t + 1].begin()->second;
          }
          // thisValue += demandAndProb[1] * value[t + 1][newState];
        }
      }
      if (thisValue < bestValue) {
        bestValue = thisValue;
        bestQ = action;
      }
    }

    std::lock_guard<std::mutex> lock(mtx); // 保护共享数据写入
    value[t][state] = bestValue;
    policy[t][state] = bestQ;
    cache_actions[state] = bestQ;
    cache_values[state] = bestValue;
  }
}

//
// 在recursion函数中使用并行计算
void NewsvendorDP::backward_parallel(const int thread_num) {
  // DpResult result;
  value.resize(T + 1);
  policy.resize(T);

  // 最后一阶段边界条件
  for (int i = static_cast<int>(min_I); i <= static_cast<int>(max_I); ++i) {
    auto state = State(static_cast<int>(T) + 1, i);
    value[T][state] = 0.0;
  }
  for (int t = static_cast<int>(T) - 1; t >= 0; --t) {
    std::vector<std::thread> threads;
    const int chunk_size = static_cast<int>((max_I - min_I) / thread_num);
    for (int threadIdx = 0; threadIdx < thread_num; ++threadIdx) {
      const int startInv = static_cast<int>(min_I) + threadIdx * chunk_size;
      const int endInv =
          (threadIdx == thread_num - 1) ? static_cast<int>(max_I) : startInv + chunk_size;
      // emplace_back 是 std::vector 提供的一个成员函数，用于在向量末尾直接构造一个新对象
      // 而不是先创建对象再插入（相比 push_back 更高效）

      // 语法为：emplace_back(func, arg)
      // 但若 func 为非静态成员函数，需要用到 this，并且前面的成员函数要用引用
      // std::ref(x) 会把 x 包装成一个可以被复制但实际是引用的对象
      // 但是原函数构造中不用 & 在大规模数据时会影响计算速度

      // std::thread 构造函数参数始终默认按值传递, 即使参数前用 & 也是，此时 & 表示指针，不是引用
      // & 在声明变量或函数构造时中跟参数表示引用，调用函数时跟参数表示指针
      threads.emplace_back(&NewsvendorDP::computeStage, this, t, startInv, endInv);
    }
    for (auto &thread : threads) {
      // 不要忘了关闭线程
      thread.join();
    }
  }
}

std::vector<std::array<int, 2>> NewsvendorDP::findsS(const bool parallel) const {
  std::vector<std::array<int, 2>> arr(T);

  if (!parallel) {
    const auto opt_table = getTable();
    bool t_recorded = false;
    int t = 1;
    for (auto row : opt_table) {
      if (static_cast<int>(row[0]) != t) {
        t = static_cast<int>(row[0]);
        t_recorded = false;
      }
      if (static_cast<int>(row[2]) >= 1e-6 && !t_recorded) {
        arr[t - 1][1] = static_cast<int>(row[1] + row[2]);
      } else if (static_cast<int>(row[2]) < 1e-6 && !t_recorded) {
        arr[t - 1][0] = static_cast<int>(row[1]);
        t_recorded = true;
      }
    }
    return arr;
  }
  for (size_t t = 0; t < policy.size(); ++t) {
    std::map ordered_cache_actions(policy[t].begin(), policy[t].end());
    // 把无序 cache_actions 里的所有元素拷贝到有序容器 ordered_cache_actions
    for (const auto &[fst, snd] : ordered_cache_actions) {
      if (fst.getPeriod() == t + 1) {
        if (snd != 0)
          arr[t][1] = static_cast<int>(fst.getInitialInventory() + snd);
        else {
          arr[t][0] = static_cast<int>(fst.getInitialInventory());
          break;
        }
      }
    }
  }
  return arr;
}

std::map<int, double> NewsvendorDP::computeGy() {
  std::map<int, double> Gy;
  compute_Gy = true;

  if (parallel) {
    std::cout << "parallel computing may cause truncated state issues" << std::endl;
    std::exit(EXIT_FAILURE); // 退出程序，返回非零状态
    // const int thread_num = 8;
    // value.clear();
    // backward_parallel(thread_num);
    // for (int y = static_cast<int>(min_I); y <= max_I; ++y) {
    //   const State ini_state{1, static_cast<double>(y)};
    //   Gy[y] = value[0].at(ini_state);
    // }
    // return Gy;
  }

  for (int y = static_cast<int>(min_I); y <= max_I; ++y) {
    const State ini_state{1, static_cast<double>(y)};
    Gy[y] = recursion_serial(ini_state);
  }
  return Gy;
}

std::vector<std::map<int, double>>
NewsvendorDP::varyParameter(const std::vector<double> &parameter) {
  const size_t N = parameter.size();
  std::vector<std::map<int, double>> result(N);
  for (size_t n = 0; n < N; n++) {
    setCapacity(parameter[n]);
    cache_values.clear();
    result[n] = computeGy();
  }
  return result;
}

int main() {

  std::vector<double> demands = {9, 23, 53, 29};
  const int T = static_cast<int>(demands.size());

  // std::vector<double> demands = {10, 20, 30, 40, 50};
  // constexpr int T = 4;
  // std::vector probs(demands.size(), 1.0 / static_cast<double>(demands.size()));

  //  constexpr double mean_demand = 30;
  //  std::vector<double> demands(T, mean_demand);
  constexpr double capacity = 60; // maximum ordering quantity
  constexpr double fix_order_cost = 500;
  constexpr double unitVariOderCost = 0;
  constexpr double unit_hold_cost = 2;
  constexpr double unit_penalty_cost = 10;
  constexpr double truncQuantile = 0.9999; // truncated quantile for the demand distribution
  constexpr double maxI = 300;             // maximum possible inventory
  constexpr double minI = -300;            // minimum possible inventory
  constexpr bool parallel =
      false; // when looking for values of s, S or computing Gy, parallel is better to be false
  constexpr double stepSize = 1.0;

  //  std::vector<double> sigma(demands.size());
  //  for (int i = 0; i < demands.size(); ++i) {
  //    sigma[i] = 0.4 * demands[i];
  //  }
  const auto pmf = PMF(truncQuantile, stepSize).getPMFPoisson(demands);
  // const auto pmf = PMF::getPMFSelfDefine(demands, probs, T);

  const State ini_state(1, 20);
  auto model = NewsvendorDP(T, capacity, stepSize, fix_order_cost, unitVariOderCost, unit_hold_cost,
                            unit_penalty_cost, truncQuantile, maxI, minI, pmf, parallel, ini_state);

  constexpr int thread_num = 8;
  double opt_value = 0.0;
  const auto start_time2 = std::chrono::high_resolution_clock::now();
  if (parallel)
    model.backward_parallel(thread_num);
  else
    opt_value = model.recursion_serial(ini_state);
  const auto end_time2 = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> duration2 = end_time2 - start_time2;
  std::cout << std::string(30, '*') << std::endl;
  std::cout << "planning horizon is " << T << " periods" << std::endl;
  if (parallel) {
    std::cout << "running time of C++ in parallel with " << thread_num << " threads is "
              << duration2 << std::endl;
    opt_value = model.value[0][ini_state];
    std::cout << "Final optimal value is: " << opt_value << std::endl;
    const auto optQ = model.policy[0][ini_state];
    std::cout << "Optimal Q is: " << optQ << std::endl;
  } else {
    std::cout << "running time of C++ in serial is " << duration2 << std::endl;
    std::cout << "Final optimal value is: " << opt_value << std::endl;
    const auto optQ = model.cache_actions[ini_state];
    std::cout << "Optimal Q in period 1 is: " << optQ << std::endl;
  }

  const auto arr1 = model.computeGy();
  // // (void)check_K_convexity(arr1, fix_order_cost);
  drawGy(arr1, -100, maxI, fix_order_cost, capacity);

  // std::vector<double> capacities;
  // for (int i = 0; i < 20; i++) {
  //   capacities.push_back(i * 10);
  // }
  // const auto arr = model.varyParameter(capacities);
  // drawGyAnimation(arr, -100, maxI, fix_order_cost, capacities);

  // std::cout << "s, S in each period are: " << std::endl;
  // const auto arr_sS = model.findsS(parallel);
  // for (const auto row : arr_sS) {
  //   for (const auto col : row) {
  //     std::cout << col << ' ';
  //   }
  //   std::cout << std::endl;
  // }

  return 0;
}