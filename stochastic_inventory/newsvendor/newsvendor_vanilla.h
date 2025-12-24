/*
 * Created by Zhen Chen on 2025/12/24.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef NEWSVENDORDP_H
#define NEWSVENDORDP_H

#include "../states/state.h"
// #include <map>
#include <unordered_map>

// struct CompareState;

class newsvendor_vanilla {
  int T;
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

  // std::unordered_map<State, double> cacheActions{};
  // std::unordered_map<State, double> cacheValues{};

  // std::map<State, double, CompareState> cacheActions{};
  // std::map<State, double, CompareState> cacheValues{};

  std::unordered_map<State, double> cacheActions{};
  std::unordered_map<State, double> cacheValues{};

public:
  newsvendor_vanilla(const int T, const int capacity, const double stepSize,
                     const double fixOrderCost, const double unitVariOrderCost,
                     const double unitHoldCost, const double unitPenaltyCost,
                     const double truncatedQuantile, const double max_I, const double min_I,
                     const std::vector<std::vector<std::vector<double>>> &pmf)
      : T(T), capacity(capacity), stepSize(stepSize), fixOrderCost(fixOrderCost),
        unitVariOrderCost(unitVariOrderCost), unitHoldCost(unitHoldCost),
        unitPenaltyCost(unitPenaltyCost), truncatedQuantile(truncatedQuantile), max_I(max_I),
        min_I(min_I), pmf(pmf) {};

  [[nodiscard]] std::vector<double> feasibleActions() const;

  [[nodiscard]] State stateTransitionFunction(const State &state, double action,
                                              double demand) const;

  [[nodiscard]] double immediateValueFunction(const State &state, double action,
                                              double demand) const;

  [[nodiscard]] double getOptAction(const State &tate);

  [[nodiscard]] auto getTable() const;

  double recursion(const State &state);
};

// ✅ 自定义比较器，没啥用，不如在 state.h 里面重载 <
// struct CompareState {
//     bool operator()(const State &p1, const State &p2) const {
//         if (p1.getPeriod() < p2.getPeriod()) {
//             return true;
//         }
//         if (p1.getPeriod() == p2.getPeriod()) {
//             if (p1.getInitialInventory() < p2.getInitialInventory()) {
//                 return true;
//             }
//             return false;
//         }
//         return false;
//     }
// };

#endif // NEWSVENDORDP_H
