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
  double fix_order_cost;
  double unit_vari_order_cost;
  double unit_hold_cost;
  double unit_penalty_cost;
  double truncated_quantile;
  double max_I;
  double min_I;
  std::vector<std::vector<std::vector<double>>> pmf;
  bool parallel;
  bool compute_Gy = false;
  State ini_state;
  std::mutex mtx; // 互斥锁保护共享数据写入

public:
  NewsvendorDP(const size_t T, const int capacity, const double stepSize,
               const double fix_order_cost, const double unit_vari_order_cost,
               const double unit_hold_cost, const double unit_penalty_cost,
               const double truncated_quantile, const double max_I, const double min_I,
               std::vector<std::vector<std::vector<double>>> pmf, const bool parallel,
               const State ini_state)
      : T(static_cast<int>(T)), capacity(capacity), stepSize(stepSize),
        fix_order_cost(fix_order_cost), unit_vari_order_cost(unit_vari_order_cost),
        unit_hold_cost(unit_hold_cost), unit_penalty_cost(unit_penalty_cost),
        truncated_quantile(truncated_quantile), max_I(max_I), min_I(min_I), pmf(std::move(pmf)),
        parallel(parallel), ini_state(ini_state) {};

  [[nodiscard]] std::vector<double> feasibleActions() const;

  [[nodiscard]] State stateTransitionFunction(const State &state, double action,
                                              double demand) const;

  [[nodiscard]] double immediateValueFunction(const State &state, double action,
                                              double demand) const;

  double getOptAction(const State &state);

  auto getTable() const;

  double recursion_serial(const State &state);

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

  [[nodiscard]] std::vector<std::array<int, 2>> findsS(bool parallel) const;

  void backward_parallel(int thread_num);
  std::vector<double> computeGy();
};

#endif // NEWSVENDOR_H
