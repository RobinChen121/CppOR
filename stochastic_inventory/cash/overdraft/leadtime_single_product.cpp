/*
 * Created by Zhen Chen on 2025/3/15.
 * Email: chen.zhen5526@gmail.com
 * Description: For 3 period, c++ is faster than java, but for 4 period, it is
 * lower (over 120s vs 41s in java). The main reason lies in the recursion for
 * double hash map. If rounding the cash in recursion, running time is about 9s
 * while java is still 41s;
 * rounding 1 decimal, running time is about 60s; no
 * rounding, running time is 128s or 800s (related with max Q capacity). Java's JIT seems to
 * optimize some computations: after running the DP sometimes in Java, its result becomes not
 * related with the number of decimals.
 *
 *
4 periods, round 2 decimals, maxQ is 100, running time is 4064.68
Final expected cash increment is 168.249
Optimal Q in the first period is 22; (price is 5)

4 periods, round 2 decimals, maxQ is 60,
running time is 94.7599
Final expected cash increment is 168.249
Optimal Q in the first period is 22. (price is 5)

demands = {9, 17, 35, 10},
4 periods, round 2 decimals, maxQ is 80,
running time is 4430.15
Final expected cash increment is 232.183
Optimal Q in the first period is 25; (price is 5)

4 periods, no rounding, maxQ is 60,
running time is 537.941
Final expected cash increment is 232.183
Optimal Q in the first period is 25. (price is 5)

4 periods, no rounding, maxQ is 60,
running time is 1798.07s, if maxQ is 50, running time is 95.9564s
Final expected cash increment is 317.59
Optimal Q in the first period is 26. (price is 10).

4 periods, no rounding, maxQ is 60,
demands = {7.0, 30.0, 15.0, 12.0}
running time is 609.356s
Final expected cash increment is 280.52
Optimal Q in the first period is 42.


 *
 *
 * 5 periods, rounding with 0 decimal(final value is 228), running time is 60.5s.
 * {15, 15, 15, 15, 15}, rounding with 1 decimal, running time is 876.99s or 1400s (no o2,o3 will be
 * more than 5000s), final value is 250.998(should be optimal), opt Q at period 1 is 24; No rounding
 * lasts 2 hours no solution.
 *
 *
 */

#include "leadtime_single_product.h"
#include <chrono>
#include <iostream>
#include <vector>

[[nodiscard]] std::vector<double> OverdraftLeadtimeSingleProduct::get_feasible_actions() const {
  const int QNum = static_cast<int>(max_order_quantity);

  std::vector<double> actions(QNum);
  for (int i = 0; i < QNum; i = i + 1) {
    actions[i] = i;
  }
  return actions;
}

[[nodiscard]] double OverdraftLeadtimeSingleProduct::immediate_value_function(
    const CashLeadtimeState &state, const double action, const double random_demand) const {
  const int t = state.get_period() - 1;
  const double revenue =
      prices[t] * std::min(state.get_ini_inventory() + state.get_q_pre(), random_demand);
  const double variable_cost = unit_vari_order_costs[t] * action;
  const double inventory = state.get_ini_inventory() + state.get_q_pre() - random_demand;
  const double cash_before_interest = state.get_ini_cash() - variable_cost - overhead_costs[t];
  double interest = 0;
  if (cash_before_interest > 0.0) {
    interest = cash_before_interest * r0;
  } else if (-overdraft_limit < cash_before_interest && cash_before_interest < 0.0) {
    interest = cash_before_interest * r1;
  } else {
    interest = -overdraft_limit * r1 + (cash_before_interest - overdraft_limit) * r2;
    // interest = cash_before_interest * r2;
  }
  const double cash_after_interest = cash_before_interest + interest + revenue;
  double cash_increment = cash_after_interest - state.get_ini_cash();
  const double salvage_value =
      state.get_period() == T ? unit_salvage_value * std::max(inventory, 0.0) : 0;
  cash_increment += salvage_value;
  return cash_increment;
}

[[nodiscard]] CashLeadtimeState OverdraftLeadtimeSingleProduct::state_transition_function(
    const CashLeadtimeState &state, const double action, const double random_demand) const {
  double next_inventory =
      std::max(0.0, state.get_ini_inventory() + state.get_q_pre() - random_demand);
  const double nextQpre = action;
  double nextCash = state.get_ini_cash() + immediate_value_function(state, action, random_demand);
  nextCash = nextCash > max_cash ? max_cash : nextCash;
  nextCash = nextCash < min_cash ? min_cash : nextCash;
  next_inventory = next_inventory > max_inventory ? max_inventory : next_inventory;
  next_inventory = next_inventory < min_inventory ? min_inventory : next_inventory;
  // cash is integer or not
  // nextCash = std::round(nextCash * 100) / 100.0; // the right should be a decimal.
  return CashLeadtimeState{state.get_period() + 1, next_inventory, nextCash, nextQpre};
}

double OverdraftLeadtimeSingleProduct::recursion(
    const CashLeadtimeState &state) { // NOLINT(*-no-recursion)
  double best_q = 0.0;
  double best_value = std::numeric_limits<double>::lowest();
  const std::vector<double> actions = get_feasible_actions();
  //  if (state.get_period() == 4) {
  //    actions = {0.0};
  //  } else {
  //    actions = get_feasible_actions();
  //  }
  for (const double action : actions) {
    double this_value = 0.0;
    for (auto demand_and_prob : pmf[state.get_period() - 1]) {
      this_value +=
          demand_and_prob[1] * immediate_value_function(state, action, demand_and_prob[0]);
      if (state.get_period() < T) {
        auto new_state = state_transition_function(state, action, demand_and_prob[0]);
        auto it = cache_values.find(new_state);
        if (it != cache_values.end()) {
          this_value += demand_and_prob[1] * it->second;
        } else {
          this_value += demand_and_prob[1] * recursion(new_state);
        }
      }
    }
    if (this_value > best_value) {
      best_value = this_value;
      best_q = action;
    }
  }
  cache_actions[state] = best_q;
  cache_values[state] = best_value;
  return best_value;
}

std::vector<double> OverdraftLeadtimeSingleProduct::solve() {
  return {recursion(ini_state), cache_actions.at(ini_state)};
}

int main() {
  auto problem = OverdraftLeadtimeSingleProduct();
  const auto start_time = std::chrono::high_resolution_clock::now();
  const auto final_value = problem.solve()[0];
  const auto end_time = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> time = end_time - start_time;
  std::cout << "running time is " << time.count() << 's' << std::endl;
  std::cout << "Final expected cash increment is " << final_value << std::endl;
  std::cout << "Optimal Q in the first period is " << problem.solve()[1] << std::endl;

  return 0;
}