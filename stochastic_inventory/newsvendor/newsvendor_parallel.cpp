/*
 * Created by Zhen Chen on 2025/3/9.
 * Email: chen.zhen5526@gmail.com
 * Description:
 * planning horizon is 40 periods, capacity 150
 * running time of C++ in serial is 4.57606s
 * Final optimal value is: 1359.28
 * running time of C++ in parallel is 1.01151s (8 threads)
 *
 * planning horizon 70 periods, capacity 100, parallel computing time is 1.3s
 *
 */

#include "../../utils/pmf.h"
#include "newsvendor.h"
#include <chrono>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <thread>

NewsvendorDP::NewsvendorDP(const size_t T, const int capacity, const double stepSize,
                           const double fixOrderCost, const double unitVariOrderCost,
                           const double unitHoldCost, const double unitPenaltyCost,
                           const double truncatedQuantile, const double max_I, const double min_I,
                           std::vector<std::vector<std::vector<double>>> pmf)
    : T(static_cast<int>(T)), capacity(capacity), stepSize(stepSize), fixOrderCost(fixOrderCost),
      unitVariOrderCost(unitVariOrderCost), unitHoldCost(unitHoldCost),
      unitPenaltyCost(unitPenaltyCost), truncatedQuantile(truncatedQuantile), max_I(max_I),
      min_I(min_I), pmf(std::move(pmf)) {};

std::vector<double> NewsvendorDP::feasibleActions() const {
  const int QNum = static_cast<int>(capacity / stepSize);
  std::vector<double> actions(QNum);
  for (int i = 0; i < QNum; i = i + 1) {
    actions[i] = i * stepSize;
  }
  return actions;
}

State NewsvendorDP::stateTransitionFunction(const State &state, const double action,
                                            const double demand) const {
  double nextInventory = state.getInitialInventory() + action - demand;
  if (state.getPeriod() == 1) {
    (void)nextInventory;
  }
  if (nextInventory > 0) {
    (void)nextInventory;
  }
  nextInventory = nextInventory > max_I ? max_I : nextInventory;
  nextInventory = nextInventory < min_I ? min_I : nextInventory;

  const int nextPeriod = state.getPeriod() + 1;
  // C++11 引入了统一的列表初始化（Uniform Initialization），鼓励使用大括号 {} 初始化类
  const auto newState = State{nextPeriod, nextInventory};

  return newState;
}

double NewsvendorDP::immediateValueFunction(const State &state, const double action,
                                            const double demand) const {
  const double fixCost = action > 0 ? fixOrderCost : 0;
  const double variCost = action * unitVariOrderCost;
  double nextInventory = state.getInitialInventory() + action - demand;
  nextInventory = nextInventory > max_I ? max_I : nextInventory;
  nextInventory = nextInventory < min_I ? min_I : nextInventory;
  const double holdCost = std::max(unitHoldCost * nextInventory, 0.0);
  const double penaltyCost = std::max(-unitPenaltyCost * nextInventory, 0.0);

  const double totalCost = fixCost + variCost + holdCost + penaltyCost;
  return totalCost;
}

double NewsvendorDP::getOptAction(const State &state) { return cache_actions[state]; }

auto NewsvendorDP::getTable() const {
  const size_t stateNums = cache_actions.size();
  std::vector<std::vector<double>> table(stateNums, std::vector<double>(3));
  int index = 0;
  for (const auto &[fst, snd] : cache_actions) {
    table[index][0] = fst.getPeriod();
    table[index][1] = fst.getInitialInventory();
    table[index][2] = snd;
    index++;
  }
  return table;
}

double NewsvendorDP::recursion(const State &state) {
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
          thisValue += demandAndProb[1] * recursion(newState);
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
    for (const std::vector<double> actions = feasibleActions(); const double action : actions) {
      double thisValue = 0;
      for (auto demandAndProb : pmf[t]) {
        thisValue += demandAndProb[1] * immediateValueFunction(state, action, demandAndProb[0]);
        if (t < T - 1) {
          auto newState = stateTransitionFunction(state, action, demandAndProb[0]);
          thisValue += demandAndProb[1] * value[t + 1][newState];
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

std::vector<std::array<int, 2>> NewsvendorDP::findsS() {
  std::vector<std::array<int, 2>> arr(T);

  for (size_t t = 0; t < policy.size(); ++t) {
    std::map<State, int> ordered_cache_actions(policy[t].begin(), policy[t].end());
    // 把无序 cache_actions 里的所有元素拷贝到有序容器 ordered_cache_actions
    for (const auto &[fst, snd] : ordered_cache_actions) {
      if (fst.getPeriod() == t + 1) {
        if (snd != 0)
          arr[t][1] = fst.getInitialInventory() + snd;
        else {
          arr[t][0] = fst.getInitialInventory();
          break;
        }
      }
    }
  }
  return arr;
}

int main() {
  std::vector<double> demands(10, 20);
  const std::string distribution_type = "poisson";
  constexpr int capacity = 100; // maximum ordering quantity
  constexpr double stepSize = 1.0;
  constexpr double fixOrderCost = 0;
  constexpr double unitVariOderCost = 1;
  constexpr double unitHoldCost = 2;
  constexpr double unitPenaltyCost = 10;
  constexpr double truncQuantile = 0.9999; // truncated quantile for the demand distribution
  constexpr double maxI = 500;             // maximum possible inventory
  constexpr double minI = -300;            // minimum possible inventory

  const auto pmf = PMF(truncQuantile, stepSize, distribution_type).getPMFPoisson(demands);
  const size_t T = demands.size();
  // auto model1 = NewsvendorDP(T, capacity, stepSize, fixOrderCost, unitVariOderCost, unitHoldCost,
  // unitPenaltyCost, truncQuantile, maxI, minI, pmf);

  const State initialState(1, 0);
  // auto start_time = std::chrono::high_resolution_clock::now();
  // auto optValue = model1.recursion(initialState);
  // auto end_time = std::chrono::high_resolution_clock::now();
  // std::chrono::duration<double> duration = end_time - start_time;
  // std::cout << "planning horizon is " << T << " periods" << std::endl;
  // std::cout << "running time of C++ in serial is " << duration <<
  // std::endl; std::cout << "Final optimal value is: " << optValue <<
  // std::endl;

  constexpr int thread_num = 8;
  auto model2 = NewsvendorDP(T, capacity, stepSize, fixOrderCost, unitVariOderCost, unitHoldCost,
                             unitPenaltyCost, truncQuantile, maxI, minI, pmf);
  auto start_time = std::chrono::high_resolution_clock::now();
  model2.backward_parallel(thread_num);
  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end_time - start_time;
  std::cout << std::string(30, '*') << std::endl;
  std::cout << "planning horizon is " << T << " periods" << std::endl;
  std::cout << "running time of C++ in parallel with " << thread_num << " threads is " << duration
            << std::endl;
  std::cout << "Final optimal value is: " << model2.value[0][initialState] << std::endl;
  const auto optQ = model2.policy[0][initialState];
  std::cout << "Optimal Q is: " << optQ << std::endl;

  std::cout << "s, S in each period are: " << std::endl;
  auto arr_sS = model2.findsS();
  for (const auto row : arr_sS) {
    for (const auto col : row) {
      std::cout << col << ' ';
    }
    std::cout << std::endl;
  }

  return 0;
}
