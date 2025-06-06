/*
 * Created by Zhen Chen on 2025/4/7.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef LEADTIME_SINGLE_PRODUCT_H
#define LEADTIME_SINGLE_PRODUCT_H
#include "../../../utils/PMF.h"
#include "../../states/CashLeadtimeState.h"

class OverdraftLeadtimeSingleProduct {
private:
  double ini_inventory = 0;
  double ini_cash = 0;
  CashLeadtimeState ini_state = CashLeadtimeState{1, ini_inventory, ini_cash, 0.0};
  std::vector<double> demands = {15.0, 15.0, 15.0, 15.0};
  std::string distribution_type = "poisson";
  size_t T = demands.size(); // 直接获取大小

  std::vector<double> prices = std::vector<double>(T, 5.0);
  // std::vector<double> fixed_order_costs = std::vector<double>(T, 0.0);
  std::vector<double> unit_vari_order_costs = std::vector<double>(T, 1.0);
  // std::vector<double> unit_hold_costs = std::vector<double>(T, 0.0);
  std::vector<double> overhead_costs = std::vector<double>(T, 50.0);
  double unit_salvage_value = 0.5; //  * unit_vari_order_costs[T - 1];

  double r0 = 0.0;
  double r1 = 0.05;
  double r2 = 2.0;
  double overdraft_limit = 300;

  double max_order_quantity = 60.0; // affect much
  double truncated_quantile = 0.9999;
  double step_size = 1.0;
  double min_inventory = 0;
  double max_inventory = 50;
  double min_cash = -200;
  double max_cash = 1000;

  std::vector<std::vector<std::vector<double>>> pmf;
  std::unordered_map<CashLeadtimeState, double> cacheActions;
  std::unordered_map<CashLeadtimeState, double> cacheValues;

public:
  OverdraftLeadtimeSingleProduct() {
    pmf = PMF(truncated_quantile, step_size, distribution_type).getPMFPoisson(demands);
  }

  OverdraftLeadtimeSingleProduct(std::vector<double> &mean_demands, double interest,
                                 double overhead_cost, double price)
      : demands(mean_demands), r1(interest) {
    overhead_costs = std::vector<double>(T, overhead_cost);
    prices = std::vector<double>(T, price);
    pmf = PMF(truncated_quantile, step_size, distribution_type).getPMFPoisson(mean_demands);
  }

  // [[nodiscard]] 表示：“函数的返回值不应该被忽略”
  // 如果你调用这个函数但没有使用返回值，编译器会给你一个警告，提醒你可能写错了。
  [[nodiscard]] std::vector<double> feasibleActions() const;
  [[nodiscard]] double immediateValueFunction(const CashLeadtimeState &state, const double action,
                                              const double randomDemand) const;
  [[nodiscard]] CashLeadtimeState stateTransitionFunction(const CashLeadtimeState &state,
                                                          const double action,
                                                          const double randomDemand) const;
  double recursion(const CashLeadtimeState &state);
  std::vector<double> solve();
};

#endif // LEADTIME_SINGLE_PRODUCT_H
