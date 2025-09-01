//
// Created by Zhen Chen on 2025/2/26.
// 40 periods, maxQ 150, value is 1359, time is 4.61s
// 40 periods, maxQ 100, value is 1351, time is 3.03s
// 这个程序无法调试了
//

#include "newsvendor.h"
#include "../../utils/pmf.h"
#include <chrono>
#include <iostream>
#include <map>

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
  // C++11 引入了统一的列表初始化（Uniform Initialization），鼓励使用大括号 {}
  // 初始化类
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

// std::vector<std::array<int, 2>> NewsvendorDP::find_sS() const {
//   return {};
// }

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
        if (auto it = cache_values.find(newState); it != cache_values.end()) {
          // some issues here
          thisValue += demandAndProb[1] * it->second;
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
  std::vector<double> demands(2, 20);
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
  auto model = NewsvendorDP(T, capacity, stepSize, fixOrderCost, unitVariOderCost, unitHoldCost,
                            unitPenaltyCost, truncQuantile, maxI, minI, pmf);

  const State initialState(1, 0);
  const auto start_time = std::chrono::high_resolution_clock::now();
  const auto optValue = model.recursion(initialState);
  const auto end_time = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> duration = end_time - start_time;
  std::cout << "planning horizon is " << T << " periods" << std::endl;
  std::cout << "running time of C++ in serial is " << duration << std::endl;
  std::cout << "Final optimal value is: " << optValue << std::endl;
  const auto optQ = model.cache_actions[initialState];
  std::cout << "Optimal Q is: " << optQ << std::endl;

  double test[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  std::cout << "cache_actions size = " << model.cache_actions.size() << std::endl;
  for (auto &kv : model.cache_actions) {
    std::cout << "key hash = " << kv.first << " value = " << kv.second << std::endl;
  }

  std::cout << "s, S in each period are: " << std::endl;
  auto arr_sS = model.findsS();
  for (const auto row : arr_sS) {
    for (const auto col : row) {
      std::cout << col << ' ';
    }
    std::cout << std::endl;
  }

  return 0;
}
