/*
 * Created by Zhen Chen on 2025/3/21.
 * Email: chen.zhen5526@gmail.com
 * Description: The DP can't output results for more than 1 hour with problems
 * of 3 periods Poission distribution and round to 1 decimal of the cash states
 * (actually it takes 8017s for the 3 periods problem where the number of decimal
 *  is 0; when it is not 0, 4 hours no result).
 *
3 periods Poisson:
std::vector<double> mean_demand1 = {15, 15, 15};
Final expected cash increment is 39.6173
Optimal Qs in the first period is 21 and 12
running time is 14172.1s (1 decimal)

 *
 * std::vector<std::vector<double>> demand1_values =
 *     std::vector(T, std::vector<double>{20.0, 30.0, 40.0});
 * std::vector<std::vector<double>> demand1_weights =
 *     std::vector(T, std::vector<double>{0.25, 0.5, 0.25});
 * std::vector<std::vector<double>> demand2_values =
 *     std::vector(T, std::vector<double>(3));
 * std::vector<std::vector<double>> demand2_weights =
 *     std::vector(T, std::vector<double>(3));
 * std::string distribution_type = "self_discrete";
 * For the self defined discrete distribution, T = 2, round 1 decimal for the
 * cash states, Final expected cash increment is -17.8, Optimal Qs in the
 * first period is 40 and 20, running time is 6.46708.
 *
 *
 *
 */

#include "../../../utils//PMF.h"
#include "../../states/CashLeadtimeMultiState.h"

#include <algorithm>
#include <iostream>
#include <vector>

class OverdraftLeadtimeDoubleProduct {
private:
  double ini_inventory1 = 0.0;
  double ini_inventory2 = 0.0;
  double ini_cash = 0.0;
  double ini_Qpre1 = 0.0;
  double ini_Qpre2 = 0.0;

  CashLeadtimeMultiState ini_state{1,         ini_inventory1, ini_inventory2,
                                   ini_Qpre1, ini_Qpre2,      ini_cash};

  std::vector<double> mean_demand1 = {15, 15, 15};
  size_t T = 2; // mean_demand1.size(); // 直接获取大小
  std::vector<double> mean_demand2 = std::vector<double>(T);
  std::string distribution_type = "poisson";

  //  std::vector<std::vector<double>> demand1_values =
  //      std::vector(T, std::vector<double>{10.0, 30.0});
  //  std::vector<std::vector<double>> demand1_weights =
  //      std::vector(T, std::vector<double>{0.5, 0.5});
  //  std::vector<std::vector<double>> demand2_values =
  //      std::vector(T, std::vector<double>(3));
  //  std::vector<std::vector<double>> demand2_weights =
  //      std::vector(T, std::vector<double>(3));
  //  std::string distribution_type = "self_discrete";

  // 直接 std::vector<double> prices1(T, 5.0); 会触发 Most Vexing
  // Parse，被解析为函数声明.
  // 通过 = 显示调用构造函数，明确告诉编译器：我们在
  // 赋值初始化成员变量，不会被解析为函数声明。
  std::vector<double> prices1 = std::vector<double>(T, 5.0);
  std::vector<double> prices2 = std::vector<double>(T, 10.0);
  std::vector<double> unit_vari_order_costs1 = std::vector<double>(T, 1.0);
  std::vector<double> unit_vari_order_costs2 = std::vector<double>(T, 2.0);
  std::vector<double> overhead_costs = std::vector<double>(T, 50.0);
  double unit_salvage_value1 = 0.5 * unit_vari_order_costs1[T - 1];
  double unit_salvage_value2 = 0.5 * unit_vari_order_costs2[T - 1];

  double r0 = 0.0;
  double r1 = 0.1;
  double r2 = 2.0;
  double overdraft_limit = 500;

  double max_order_quantity1 = 25.0;
  double max_order_quantity2 = 15.0;
  double truncated_quantile = 0.9999;
  double step_size = 1.0;
  double min_inventory = 0;
  double max_inventory = 50;
  double min_cash = -200;
  double max_cash = 300;

  std::vector<std::vector<std::vector<double>>> pmf;
  std::unordered_map<CashLeadtimeMultiState, std::array<double, 2>> cacheActions;
  std::unordered_map<CashLeadtimeMultiState, double> cacheValues;

public:
  // std::transform
  // 需要在运行时执行，而类的成员变量必须在构造函数的初始化列表或构造函数体内初始化
  // 因此 transform 不能在类的私有属性中使用
  OverdraftLeadtimeDoubleProduct() {

    std::ranges::transform(mean_demand1, mean_demand2.begin(),
                           [](const double x) { return x / 2; });
    pmf = PMF(truncated_quantile, step_size, distribution_type)
              .getPMFPoissonMulti(mean_demand1, mean_demand2);

    //    if (distribution_type == "self_discrete") {
    //      for (int t = 0; t < T; ++t) {
    //        std::ranges::transform(demand1_values[t], demand2_values[t].begin(),
    //                               [](const double x) { return x / 2; });
    //      }
    //      for (int t = 0; t < T; ++t) {
    //        std::ranges::transform(demand1_weights[t], demand2_weights[t].begin(),
    //                               [](const double x) { return x; });
    //      }
    //      pmf = PMF(truncated_quantile, step_size, distribution_type)
    //                .getPMFSelfDiscreteMulti(demand1_values, demand1_weights,
    //                                         demand2_values, demand2_weights);
    //      ;
    //    }
  }

  [[nodiscard]] std::vector<std::array<double, 2>> feasibleActions() const {
    const int QNum1 = static_cast<int>(max_order_quantity1 / step_size);
    const int QNum2 = static_cast<int>(max_order_quantity2 / step_size);
    const int QTotalNums = QNum1 * QNum2;
    int index = 0;
    std::vector<std::array<double, 2>> actions(QTotalNums);
    for (int i = 0; i < QNum1; i = i + 1) {
      const double action1 = i * step_size;
      for (int j = 0; j < QNum2; j = j + 1) {
        const double action2 = j * step_size;
        actions[index] = {action1, action2};
        index += 1;
      }
    }
    return actions;
  }

  [[nodiscard]] double immediateValueFunction(const CashLeadtimeMultiState &state,
                                              const double action1, const double action2,
                                              const double randomDemand1,
                                              const double randomDemand2) const {
    const int t = state.getPeriod() - 1;
    const double revenue =
        prices1[t] * std::min(state.getIniI1() + state.getQpre1(), randomDemand1) +
        prices2[t] * std::min(state.getIniI2() + state.getQpre2(), randomDemand2);
    const double variableCost =
        unit_vari_order_costs1[t] * action1 + unit_vari_order_costs2[t] * action2;
    const double inventoryLevel1 = state.getIniI1() + state.getQpre1() - randomDemand1;
    const double inventoryLevel2 = state.getIniI2() + state.getQpre2() - randomDemand2;
    const double cash_before_interest = state.getIniCash() - variableCost - overhead_costs[t];
    double interest = 0;
    if (cash_before_interest > 0.0) {
      interest = cash_before_interest * r0;
    } else if (-overdraft_limit < cash_before_interest && cash_before_interest < 1e-6) {
      interest = cash_before_interest * r1;
    } else {
      interest = -overdraft_limit * r1 + (cash_before_interest - overdraft_limit) * r2;
      //      interest = cash_before_interest * r2;
    }
    const double cash_after_interest = cash_before_interest + interest + revenue;
    double cash_increment = cash_after_interest - state.getIniCash();
    const double salValue = state.getPeriod() == T
                                ? unit_salvage_value1 * std::max(inventoryLevel1, 0.0) +
                                      unit_salvage_value2 * std::max(inventoryLevel2, 0.0)
                                : 0;
    cash_increment += salValue;
    return cash_increment;
  }

  [[nodiscard]] CashLeadtimeMultiState stateTransitionFunction(const CashLeadtimeMultiState &state,
                                                               const double action1,
                                                               const double action2,
                                                               const double randomDemand1,
                                                               const double randomDemand2) const {
    double nextInventory1 = std::max(0.0, state.getIniI1() + state.getQpre1() - randomDemand1);
    double nextInventory2 = std::max(0.0, state.getIniI2() + state.getQpre2() - randomDemand2);
    const double nextQpre1 = action1;
    const double nextQpre2 = action2;
    double nextCash = state.getIniCash() +
                      immediateValueFunction(state, action1, action2, randomDemand1, randomDemand2);
    nextCash = nextCash > max_cash ? max_cash : nextCash;
    nextCash = nextCash < min_cash ? min_cash : nextCash;
    nextInventory1 = nextInventory1 > max_inventory ? max_inventory : nextInventory1;
    nextInventory1 = nextInventory1 < min_inventory ? min_inventory : nextInventory1;
    nextInventory2 = nextInventory2 > max_inventory ? max_inventory : nextInventory2;
    nextInventory2 = nextInventory2 < min_inventory ? min_inventory : nextInventory2;
    // cash is integer or not
    nextCash = std::round(nextCash * 10) / 10.0; // the right should be a
    // decimal
    return CashLeadtimeMultiState{
        state.getPeriod() + 1, nextInventory1, nextInventory2, nextQpre1, nextQpre2, nextCash};
  }

  double recursion(const CashLeadtimeMultiState &state) { // NOLINT(*-no-recursion)
    std::array bestQ = {0.0, 0.0};
    double bestValue = std::numeric_limits<double>::lowest();
    for (const std::vector<std::array<double, 2>> actions = feasibleActions();
         const std::array action : actions) {
      double thisValue = 0.0;
      // if (state.getPeriod() == 2 and state.getQpre1() == 15 and
      //     state.getQpre2() == 0) {
      //   ;
      // }
      for (auto demandAndProb : pmf[state.getPeriod() - 1]) {
        thisValue += demandAndProb[2] * immediateValueFunction(state, action[0], action[1],
                                                               demandAndProb[0], demandAndProb[1]);
        if (state.getPeriod() < T) {
          auto newState = stateTransitionFunction(state, action[0], action[1], demandAndProb[0],
                                                  demandAndProb[1]);
          if (auto it = cacheValues.find(newState); it != cacheValues.end()) {
            // some issues here
            thisValue += demandAndProb[2] * it->second;
          } else {
            thisValue += demandAndProb[2] * recursion(newState);
          }
        }
      }
      if (thisValue > bestValue) {
        bestValue = thisValue;
        bestQ = action;
      }
    }
    cacheActions[state] = bestQ;
    cacheValues[state] = bestValue;
    return bestValue;
  }

  std::vector<double> solve() {
    return {recursion(ini_state), cacheActions.at(ini_state)[0], cacheActions.at(ini_state)[1]};
  }
};

// std::ostream operator<<(const std::ostream & lhs, const
// std::chrono::duration<double> & rhs);
int main() {
  auto problem = OverdraftLeadtimeDoubleProduct();
  const auto start_time = std::chrono::high_resolution_clock::now();
  const auto final_value = problem.solve()[0];
  const auto end_time = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> time = end_time - start_time;
  std::cout << "Final expected cash increment is " << final_value << std::endl;
  std::cout << "Optimal Qs in the first period is " << problem.solve()[1] << " and "
            << problem.solve()[2] << std::endl;
  std::cout << "running time is " << time.count() << std::endl;

  return 0;
}