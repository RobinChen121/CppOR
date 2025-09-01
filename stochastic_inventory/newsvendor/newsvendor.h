/*
 * Created by Zhen Chen on 2025/3/9.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef NEWSVENDOR_H
#define NEWSVENDOR_H

#include "../states/state.h"
#include <mutex>
#include <unordered_map>
#include <vector>

class NewsvendorDP {
  size_t T;
  int capacity;
  double stepSize;
  double fixOrderCost;
  double unitVariOrderCost;
  double unitHoldCost;
  double unitPenaltyCost;
  double truncatedQuantile;
  double max_I;
  double min_I;
  std::vector<std::vector<std::vector<double>>> pmf;
  std::mutex mtx; // 互斥锁保护共享数据写入

public:
  NewsvendorDP(size_t T, int capacity, double stepSize, double fixOrderCost,
               double unitVariOrderCost, double unitHoldCost, double unitPenaltyCost,
               double truncatedQuantile, double max_I, double min_I,
               std::vector<std::vector<std::vector<double>>> pmf);

  [[nodiscard]] std::vector<double> feasibleActions() const;

  [[nodiscard]] State stateTransitionFunction(const State &state, double action,
                                              double demand) const;

  [[nodiscard]] double immediateValueFunction(const State &state, double action,
                                              double demand) const;

  double getOptAction(const State &state);

  auto getTable() const;

  double recursion(const State &state);

  double recursion_parallel(const State &state);

  void computeStage(int t, int start_inventory, int end_inventory);

  // struct DpResult {
  //   std::vector<std::unordered_map<State, double>> value;  // V[t][inventory]
  //   std::vector<std::unordered_map<State, double>> policy; // policy[t][inventory]
  // };

  std::unordered_map<State, double> cache_actions;
  std::unordered_map<State, double> cache_values;

  // for parallel
  std::vector<std::unordered_map<State, double>> value;  // V[t][inventory]
  std::vector<std::unordered_map<State, double>> policy; // policy[t][inventory]

  std::vector<std::array<int, 2>> findsS();

  void backward_parallel(const int thread_num);
};

#endif // NEWSVENDOR_H
