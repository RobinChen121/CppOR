/*
 * Created by Zhen Chen on 2025/6/17.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef WORKFORCE_PLAN_H
#define WORKFORCE_PLAN_H

#include "../../utils/draw_graph.h"
#include "../../utils/pmf.h"
#include "worker_state.h"
#include <chrono>
// #include <iostream>
// #include <map>
#include <mutex>
// #include <thread>
#include <unordered_map>

enum class Direction { FORWARD, BACKWARD };
enum class ToComputeGy { True, False };

class WorkforcePlan {
  Direction direction = Direction::BACKWARD;
  ToComputeGy to_compute_gy = ToComputeGy::False;

  std::vector<double> turnover_rates = {0.5, 0.3, 0.5};
  size_t T = turnover_rates.size();

  int initial_workers = 0;
  // 类初始化 {} 更安全，防止类属性窄化，例如从 double 到 int 这样的精度丢失
  WorkerState ini_state = WorkerState{1, initial_workers};
  double fix_hire_cost = 100.0;
  double unit_vari_cost = 0.0;
  double salary = 20.0;
  double unit_penalty = 80.0;
  // 初始化给定默认值时就可以使用已声明变量的值
  std::vector<int> min_workers = std::vector<int>(T, 40);

  int piece_segment = 10;

  std::string varied_parameter;
  std::string K_convexity;
  std::string binomial_K_convexity;
  std::string convexity;

  int max_hire_num = 500;
  int max_worker_num = 500;

  std::vector<std::vector<double>> p_c; // cumulative binomial probability
  std::vector<std::vector<std::vector<double>>> pmf;
  // std::vector<std::vector<std::vector<std::array<double, 2>>>> pmf2;
  std::unordered_map<WorkerState, double> cache_actions;
  std::unordered_map<WorkerState, double> cache_values;

  std::mutex mtx; // 互斥锁保护共享数据写入

public:
  std::vector<std::unordered_map<WorkerState, double>> values;
  std::vector<std::unordered_map<WorkerState, double>> policies;

  WorkforcePlan() {
    pmf = PMF::getPMFBinomial(max_worker_num, turnover_rates);
    compute_turnover();
    // pmf2 = PMF::getPMFBinomial2(max_worker_num, turnover_rates);
  }

  void set_fix_cost(double value);
  void set_salary(double value);
  void set_penalty(double value);
  void set_min_workers(int value);
  void set_turnover_rate(double value);
  [[nodiscard]] std::string get_varied_parameter() { return varied_parameter; };
  [[nodiscard]] std::string getKConvexity() { return K_convexity; };
  [[nodiscard]] std::string getBinomialConvexity() { return binomial_K_convexity; };
  [[nodiscard]] std::string getConvexity() { return convexity; };
  [[nodiscard]] Direction get_direction() const { return direction; };
  [[nodiscard]] WorkerState get_initial_state() const { return ini_state; };
  [[nodiscard]] int get_T() const { return static_cast<int>(T); };
  [[nodiscard]] double get_fix_hire_cost() const { return fix_hire_cost; };

  [[nodiscard]] std::vector<int> get_feasible_actions() const;
  [[nodiscard]] double immediate_value(WorkerState ini_state, int action, int overturn_num) const;
  [[nodiscard]] WorkerState state_transition(WorkerState ini_state, int action,
                                             int overturn_num) const;
  double recursion_forward(WorkerState ini_state);
  void recursion_backward_parallel();
  void compute_stage(int t, int start_inventory, int end_inventory,
                     std::vector<std::unordered_map<WorkerState, double>> &values,
                     std::vector<std::unordered_map<WorkerState, double>> &policies);
  std::vector<double> solve(WorkerState ini_state);
  [[nodiscard]] std::vector<std::array<int, 2>> find_sS() const;
  [[nodiscard]] double simulate_sS(WorkerState ini_state,
                                   const std::vector<std::array<int, 2>> &sS) const;
  [[nodiscard]] std::vector<double> compute_Gy();
  [[nodiscard]] std::vector<std::vector<double>>
  compute_expect_Gy(const std::vector<double> &Gy) const;
  [[nodiscard]] std::vector<std::vector<double>> get_opt_table() const;

  [[nodiscard]] std::pair<double, std::vector<std::array<int, 2>>> solve_mip() const;
  [[nodiscard]] std::pair<double, std::vector<std::array<int, 2>>> solve_tsp() const;

  void compute_turnover();
  [[nodiscard]] std::vector<double> compute_V() const;
  [[nodiscard]] double compute_Ltj_y(int t, int j, int y) const;
  [[nodiscard]] int find_y_star(int t, int j) const;

  bool check_K_convexity(const std::vector<double> &Gy);
  bool check_Binomial_KConvexity(const std::vector<double> &Gy,
                                 const std::vector<std::vector<double>> &expect_Gy);
  bool check_convexity(const std::vector<double> &Gy);
};
#endif // WORKFORCE_PLAN_H
