/*
 * Created by Zhen Chen on 2026/9/1.
 * Email: chen.zhen5526@gmail.com
 * Description: using 2D vector to compute the DP, which is faster than using unordered_map.
 * using 2D vector is 0.077s; (40 periods, mean demand 20, capacity 150, fix_order_cost 0,
 * unit_order_cost 1, hold_cost 2, penalty_cost 10, truncQuantile 0.9999) using 1D vector can be
 * much faster 0.056s; open parallel computation can speed up further to 0.017s; using pointer to
 * access vector values can be 0.04s, which does not significantly improve performance.
 *
 * 40 periods, running time under serial computing is 0.035s(dell), a little faster than Julia
 * 0.048s.
 *
 * if precomputing the expected cost for each inventory level, the running time can be further
 * reduced to 0.002s without parallel computation.
 *
 * If the integer values are not used in arithmetic operations, it is better to use int8_t;
 * otherwise, no need to use int8_t.
 *
 * Whether using boost does not affect computation too much.
 *
 * boost poisson 20 quantile for 0.0001 outputs 5, which should be 6 by my codes and many other
 * software such as Java, Matlab, Scipy, Mathematica.
 *
 *
 *
 *  General both the map and 1D vector implementation of the DP.
 *
 */

#ifndef WORKFORCE_NEWSVENDOR_GENERAL_H
#define WORKFORCE_NEWSVENDOR_GENERAL_H

#include <unordered_map>
#include <vector>

// get the probability mass function value of Poisson
double poissonPmf(int k, int lambda);
// get cumulative distribution function value of Poisson
double poissonCdf(const int k, const int lambda);
// get inverse cumulative distribution function value of Poisson
int poissonQuantile(const double p, const int lambda);

struct PMFData {
  std::vector<int> demands;       // flattened demand
  std::vector<double> prob;       // flattened probability
  std::vector<int> start_index_t; // start index of each t in flat arrays
  std::vector<int> len_t;         // length per t
};

PMFData getPMFPoisson(const std::vector<int> &demands, double truncated_quantile);

std::vector<std::vector<std::array<double, 2>>> getPMFPoisson2(const std::vector<int> &demands,
                                                               double truncated_quantile);

class State {
  int period{}; // c++11, {} 值初始化，默认为 0
  double ini_inventory{};

public:
  State() = default;

  explicit State(const int period, const double ini_inventory)
      : period(period), ini_inventory(ini_inventory) {};

  [[nodiscard]] double getIniInventory() const { return ini_inventory; }
  [[nodiscard]] int getPeriod() const { return period; }

  // for unordered map
  bool operator==(const State &other) const {
    return period == other.period && ini_inventory == other.ini_inventory;
  }

  friend struct std::hash<State>;
};

// 自定义哈希
template <> struct std::hash<State> {
  // size_t 表示无符号整数
  size_t operator()(const State &s) const noexcept {
    // noexcept 表示这个函数不会抛出异常
    // 计算哈希值
    // std::hash<int>() 是一个 std::hash<int> 类型的对象，调用 () 运算符可以计算
    // obj.id（整数）的哈希值
    // ^（异或）是位运算，不会造成进位，适合合并多个哈希值
    // 这里的 << 1 左移 1 位（相当于乘
    // 2），让哈希值更加分散，避免简单叠加导致哈希冲突
    return std::hash<int>()(s.period) ^ std::hash<double>()(s.ini_inventory) << 1;
  }
};

class Newsvendor {
  int T = 40;
  int mean_demand = 20;
  std::vector<int> demands = std::vector(T, mean_demand);
  // const std::vector<double> demands = {10.0, 20, 10, 20, 10, 20, 10, 20};
  // const int T = demands.size();

  int capacity = 150;
  double fix_order_cost = 0.0;
  double unit_order_cost = 1.0;
  double hold_cost = 2.0;
  double penalty_cost = 10.0;
  double truncQuantile = 0.9999;
  double ini_inventory = 0.0;

  double INF = 1e100;

  int min_I = -100;
  int max_I = 100;
  int num_inv = max_I - min_I + 1; // number of possible inventory levels

  PMFData pmf;

  std::vector<double> value{std::vector((T + 1) * num_inv, 0.0)};
  std::vector<int> policy{std::vector(T * num_inv, 0)};

  // map 用的
  std::unordered_map<State, double> cache_actions;
  std::unordered_map<State, double> cache_values;
  std::vector<int> feasible_actions{std::vector(capacity + 1, 0)};
  std::vector<std::vector<std::array<double, 2>>> pmf2;

public:
  Newsvendor() { pmf = getPMFPoisson(demands, truncQuantile); }

  explicit Newsvendor(State state) {
    pmf2 = getPMFPoisson2(demands, truncQuantile);
    feasible_actions = getFeasibleActions();
  }

  int getT() const { return T; }
  double getIniInventory() const { return ini_inventory; }

  std::pair<double, double> DP1DVector();
  // [[nodiscard]] 表示函数的返回值不应该被忽略，否则编译器会发出警告
  [[nodiscard]] std::vector<double> computeHoldPenaltyCost() const;
  [[nodiscard]] std::vector<double> computeExpectCost(int t) const;

  // map 外用的
  [[nodiscard]] State stateTransitionFunction(const State &state, double action,
                                              double demand) const;
  double immediateValueFunction(const State &state, double action, double demand) const;
  double recursion(const State &state);
  std::vector<int> getFeasibleActions();
  int getOptQ() { return cache_actions[State{1, ini_inventory}]; };
};

#endif // WORKFORCE_NEWSVENDOR_GENERAL_H
