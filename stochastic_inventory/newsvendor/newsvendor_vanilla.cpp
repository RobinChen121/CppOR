/*
 * Created by Zhen Chen on 2025/12/24.
 * Email: chen.zhen5526@gmail.com
 * Description: vanilla version of DP for newsvendor;
 *
 * 40 periods, running time under serial computing is 0.29s(dell):
* std::vector<double> demands(40, 20);
  const std::string distribution_type = "poisson";
  constexpr int capacity = 150; // maximum ordering quantity
  constexpr double stepSize = 1.0;
  constexpr double fix_order_cost = 0;
  constexpr double unitVariOderCost = 1;
  constexpr double unit_hold_cost = 2;
  constexpr double unit_penalty_cost = 10;
  constexpr double truncQuantile = 0.9999; // truncated quantile for the demand distribution
  constexpr double maxI = 100;             // maximum possible inventory
  constexpr double minI = -100;            // minimum possible inventory
 *
 *
 */

#include <array>
#include <boost/functional/hash.hpp>
#include <chrono>
#include <iostream>
#include <limits>
#include <unordered_map>

double poissonCDF(const int k, const double lambda) {
  double cumulative = 0.0;
  double term = std::exp(-lambda); // P(X=0)
  for (int i = 0; i <= k; ++i) {
    cumulative += term;
    if (i < k)
      term *= lambda / (i + 1); // 递推计算 P(X=i)
  }

  return cumulative;
}

double poissonPMF(const int k, const int lambda) {
  if (k < 0 || lambda < 0)
    return 0.0; // 确保参数合法
  if (k == 0 and lambda == 0)
    return 1.0;
  // lgamma 对 tgamma 取 ln
  const double logP = -lambda + k * std::log(lambda) - std::lgamma(k + 1);
  return std::exp(logP); // Use the logarithmic form to avoid overflow from std::tgamma(k + 1)
}

int poissonQuantile(const double p, const double lambda) {
  int low = 0, high = std::max(100, static_cast<int>(lambda * 3)); // 初始搜索区间
  while (low < high) {
    if (const int mid = (low + high) / 2; poissonCDF(mid, lambda) < p) {
      low = mid + 1;
    } else {
      high = mid;
    }
  }
  return low;
}

std::vector<std::vector<std::array<double, 2>>> getPMFPoisson(const std::vector<double> &demands,
                                                              const double truncated_quantile) {
  const size_t T = demands.size();
  std::vector<int> support_lb(T);
  std::vector<int> support_ub(T);
  for (size_t i = 0; i < T; ++i) {
    support_ub[i] = poissonQuantile(truncated_quantile, demands[i]);
    support_lb[i] = poissonQuantile(1 - truncated_quantile, demands[i]);
  }
  std::vector pmf(T, std::vector<std::array<double, 2>>());
  for (int t = 0; t < T; ++t) {
    const int demand_length = static_cast<int>((support_ub[t] - support_lb[t] + 1));
    pmf[t].resize(demand_length, std::array<double, 2>());
    for (int j = 0; j < demand_length; ++j) {
      pmf[t][j][0] = support_lb[t] + j;
      const int demand = static_cast<int>(pmf[t][j][0]);
      pmf[t][j][1] =
          poissonPMF(demand, static_cast<int>(demands[t])) / (2 * truncated_quantile - 1);
    }
  }
  return pmf;
}

class State {
  int period{}; // c++11, {} 值初始化，默认为 0
  double ini_inventory{};

public:
  State() {}

  explicit State(const int period, const double ini_inventory)
      : period(period), ini_inventory(ini_inventory) {};

  [[nodiscard]] double get_ini_inventory() const { return ini_inventory; }

  [[nodiscard]] int getPeriod() const { return period; }

  // for unordered map
  bool operator==(const State &other) const {
    return period == other.period && ini_inventory == other.ini_inventory;
  }

  friend struct std::hash<State>;

  // for ordered map
  bool operator<(const State &other) const {
    if (period != other.period)
      return period < other.period;
    if (ini_inventory != other.ini_inventory)
      return ini_inventory < other.ini_inventory;
    return false;
  }

  friend std::ostream &operator<<(std::ostream &os, const State &state);
};

template <> struct std::hash<State> {
  // size_t 表示无符号整数
  size_t operator()(const State &s) const noexcept {
    // noexcept 表示这个函数不会抛出异常
    // boost 的哈希计算更安全
    std::size_t seed = 0;
    boost::hash_combine(seed, s.period);
    boost::hash_combine(seed, s.ini_inventory);
    return seed;
  }
};

class NewsvendorDP {
  size_t T;
  double capacity;
  double fix_order_cost;
  double unit_vari_order_cost;
  double unit_hold_cost;
  double unit_penalty_cost;
  double truncated_quantile;
  double max_I;
  double min_I;
  std::vector<std::vector<std::array<double, 2>>> pmf;
  bool parallel{};
  bool compute_Gy = false;
  State ini_state{};
  std::unordered_map<State, double> cache_actions;
  std::unordered_map<State, double> cache_values;

public:
  NewsvendorDP(const size_t T, const double capacity, const double fix_order_cost,
               const double unit_vari_order_cost, const double unit_hold_cost,
               const double unit_penalty_cost, const double truncated_quantile, const double max_I,
               const double min_I, const std::vector<std::vector<std::array<double, 2>>> &pmf)
      : T(static_cast<int>(T)), capacity(capacity), fix_order_cost(fix_order_cost),
        unit_vari_order_cost(unit_vari_order_cost), unit_hold_cost(unit_hold_cost),
        unit_penalty_cost(unit_penalty_cost), truncated_quantile(truncated_quantile), max_I(max_I),
        min_I(min_I), pmf(pmf) {};

  [[nodiscard]] std::vector<double> feasibleActions() const {
    const int QNum = static_cast<int>(capacity);
    std::vector<double> actions(QNum);
    for (int i = 0; i < QNum; i = i + 1) {
      actions[i] = i;
    }
    return actions;
  }

  [[nodiscard]] State stateTransitionFunction(const State &state, const double action,
                                              const double demand) const {
    double nextInventory = state.get_ini_inventory() + action - demand;

    nextInventory = nextInventory > max_I ? max_I : nextInventory;
    nextInventory = nextInventory < min_I ? min_I : nextInventory;

    const int nextPeriod = state.getPeriod() + 1;
    // C++11 引入了统一的列表初始化（Uniform Initialization），鼓励使用大括号 {} 初始化类
    const auto newState = State{nextPeriod, nextInventory};

    return newState;
  }

  [[nodiscard]] double immediateValueFunction(const State &state, const double action,
                                              const double demand) const {
    const double fixCost = action > 0 ? fix_order_cost : 0;
    const double variCost = action * unit_vari_order_cost;
    double nextInventory = state.get_ini_inventory() + action - demand;
    nextInventory = nextInventory > max_I ? max_I : nextInventory;
    nextInventory = nextInventory < min_I ? min_I : nextInventory;
    const double holdCost = std::max(unit_hold_cost * nextInventory, 0.0);
    const double penaltyCost = std::max(-unit_penalty_cost * nextInventory, 0.0);

    const double totalCost = fixCost + variCost + holdCost + penaltyCost;
    return totalCost;
  }

  double recursion(const State &state) { // NOLINT
    double bestQ = 0.0;
    double bestValue = std::numeric_limits<double>::max();
    const std::vector<double> actions = feasibleActions(); // should not move inside
    for (const double action : actions) {
      double thisValue = 0;
      for (auto demandAndProb : pmf[state.getPeriod() - 1]) {
        thisValue += demandAndProb[1] * immediateValueFunction(state, action, demandAndProb[0]);
        if (state.getPeriod() < T) {
          auto newState = stateTransitionFunction(state, action, demandAndProb[0]);
          auto it = cache_values.find(newState);
          if (it != cache_values.end()) {
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
};

int main() {
  constexpr int T = 40;
  constexpr double mean_demand = 20;
  const std::vector demands(T, mean_demand);

  constexpr double capacity = 150; // maximum ordering quantity
  constexpr double fix_order_cost = 0;
  constexpr double unitVariOderCost = 1;
  constexpr double unit_hold_cost = 2;
  constexpr double unit_penalty_cost = 10;
  constexpr double truncQuantile = 0.9999; // truncated quantile for the demand distribution
  constexpr double maxI = 100;             // maximum possible inventory
  constexpr double minI = -100;            // minimum possible inventory

  const auto pmf = getPMFPoisson(demands, truncQuantile);
  auto model = NewsvendorDP(T, capacity, fix_order_cost, unitVariOderCost, unit_hold_cost,
                            unit_penalty_cost, truncQuantile, maxI, minI, pmf);

  const auto initialState = State(1, 0);
  const auto start_time = std::chrono::high_resolution_clock::now();
  const auto optValue = model.recursion(initialState);
  const auto end_time = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> duration = end_time - start_time;
  std::cout << "planning horizon is " << T << " periods" << std::endl;
  std::cout << "running time of C++ is " << duration << std::endl;
  std::cout << "Final optimal value is: " << optValue << std::endl;
  return 0;
}