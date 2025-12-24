/*
 * Created by Zhen Chen on 2025/12/24.
 * Email: chen.zhen5526@gmail.com
 * Description: vanilla version of DP for newsvendor;
 *
 * 40 periods, running time under serial computing is 1.17s:
* std::vector<double> demands(40, 20);
  const std::string distribution_type = "poisson";
  constexpr int capacity = 150; // maximum ordering quantity
  constexpr double stepSize = 1.0;
  constexpr double fixOrderCost = 0;
  constexpr double unitVariOderCost = 1;
  constexpr double unitHoldCost = 2;
  constexpr double unitPenaltyCost = 10;
  constexpr double truncQuantile = 0.9999; // truncated quantile for the demand distribution
  constexpr double maxI = 100;             // maximum possible inventory
  constexpr double minI = -100;            // minimum possible inventory
 *
 *
 */

#include "newsvendor_vanilla.h"

#include <chrono>
#include <iostream>

#include "../../utils/pmf.h"
#include <limits>

std::vector<double> newsvendor_vanilla::feasibleActions() const {
  const int QNum = static_cast<int>(capacity / stepSize);
  std::vector<double> actions(QNum);
  for (int i = 0; i < QNum; i = i + 1) {
    actions[i] = i * stepSize;
  }
  return actions;
}

State newsvendor_vanilla::stateTransitionFunction(const State &state, const double action,
                                                  const double demand) const {
  double nextInventory = state.get_ini_inventory() + action - demand;

  nextInventory = nextInventory > max_I ? max_I : nextInventory;
  nextInventory = nextInventory < min_I ? min_I : nextInventory;

  const int nextPeriod = state.getPeriod() + 1;
  // C++11 引入了统一的列表初始化（Uniform Initialization），鼓励使用大括号 {} 初始化类
  const auto newState = State{nextPeriod, nextInventory};

  return newState;
}

double newsvendor_vanilla::immediateValueFunction(const State &state, const double action,
                                                  const double demand) const {
  const double fixCost = action > 0 ? fixOrderCost : 0;
  const double variCost = action * unitVariOrderCost;
  double nextInventory = state.get_ini_inventory() + action - demand;
  nextInventory = nextInventory > max_I ? max_I : nextInventory;
  nextInventory = nextInventory < min_I ? min_I : nextInventory;
  const double holdCost = std::max(unitHoldCost * nextInventory, 0.0);
  const double penaltyCost = std::max(-unitPenaltyCost * nextInventory, 0.0);

  const double totalCost = fixCost + variCost + holdCost + penaltyCost;
  return totalCost;
}

double newsvendor_vanilla::getOptAction(const State &state) { return cacheActions[state]; }

auto newsvendor_vanilla::getTable() const {
  const size_t stateNums = cacheActions.size();
  std::vector<std::vector<double>> table(stateNums, std::vector<double>(3));
  int index = 0;
  for (const auto &[fst, snd] : cacheActions) {
    table[index][0] = fst.getPeriod();
    table[index][1] = fst.get_ini_inventory();
    table[index][2] = snd;
    index++;
  }
  return table;
}

double newsvendor_vanilla::recursion(const State &state) { // NOLINT
  double bestQ = 0.0;
  double bestValue = std::numeric_limits<double>::max();
  const std::vector<double> actions = feasibleActions(); // should not move inside
  for (const double action : actions) {
    double thisValue = 0;
    for (auto demandAndProb : pmf[state.getPeriod() - 1]) {
      thisValue += demandAndProb[1] * immediateValueFunction(state, action, demandAndProb[0]);
      if (state.getPeriod() < T) {
        auto newState = stateTransitionFunction(state, action, demandAndProb[0]);
        if (cacheValues.contains(newState)) {
          // some issues here
          thisValue += demandAndProb[1] * cacheValues[newState];
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
  cacheActions[state] = bestQ;
  cacheValues[state] = bestValue;
  return bestValue;
}

int main() {
  constexpr int T = 40;
  constexpr double mean_demand = 20;
  std::vector demands(T, mean_demand);

  constexpr double capacity = 150; // maximum ordering quantity
  constexpr double stepSize = 1.0;
  constexpr double fixOrderCost = 0;
  constexpr double unitVariOderCost = 1;
  constexpr double unitHoldCost = 2;
  constexpr double unitPenaltyCost = 10;
  constexpr double truncQuantile = 0.9999; // truncated quantile for the demand distribution
  constexpr double maxI = 100;             // maximum possible inventory
  constexpr double minI = -100;            // minimum possible inventory

  const auto pmf = PMF(truncQuantile, stepSize).getPMFPoisson(demands);
  auto model = newsvendor_vanilla(T, capacity, stepSize, fixOrderCost, unitVariOderCost,
                                  unitHoldCost, unitPenaltyCost, truncQuantile, maxI, minI, pmf);

  const auto initialState = State(1, 0);
  const auto start_time = std::chrono::high_resolution_clock::now();
  const auto optValue = model.recursion(initialState);
  const auto end_time = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> duration = end_time - start_time;
  std::cout << "planning horizon is " << T << " periods" << std::endl;
  std::cout << "running time of C++ is " << duration << std::endl;
  std::cout << "Final optimal value is: " << optValue << std::endl;
  // const auto optQ = model.getOptAction(initialState);
  // std::cout << "Optimal Q is: " << optQ << std::endl;
  // auto table = model.getTable();
  return 0;
}