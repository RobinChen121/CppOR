/*
 * Created by Zhen Chen on 2025/6/17.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef WORKFORCE_PLAN_H
#define WORKFORCE_PLAN_H

#include "../../utils/common.h"
#include "../../utils/draw_graph.h"
#include "../../utils/pmf.h"
#include "worker_state.h"
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_map>

enum class Direction { FORWARD, BACKWARD };
enum class ToComputeGy { True, False };

class WorkforcePlan {
  Direction direction = Direction::BACKWARD;
  ToComputeGy to_compute_gy = ToComputeGy::False;

  std::vector<double> turnover_rates = {0.6, 0.5, 0.4};
  size_t T = turnover_rates.size();

  int initial_workers = 0;
  // 类初始化 {} 更安全，防止类属性窄化，例如从 double 到 int 这样的精度丢失
  WorkerState ini_state = WorkerState{1, initial_workers};
  double fix_hire_cost = 50.0;
  double unit_vari_cost = 0;
  double salary = 30.0;
  double unit_penalty = 40.0;
  std::vector<int> min_workers = std::vector<int>(T, 40);

  int piece_segment = 10;

  std::string varied_parameter;
  std::string K_convexity;
  std::string binomial_K_convexity;
  std::string convexity;

  int max_hire_num = 500;
  int max_worker_num = 600;

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
    // pmf2 = PMF::getPMFBinomial2(max_worker_num, turnover_rates);
  }

  void set_fix_cost(const double value);
  void set_salary(const double value);
  void set_penalty(const double value);
  void set_min_workers(const int value);
  void set_turnover_rate(const double value);
  [[nodiscard]] std::string get_varied_parameter() { return varied_parameter; };
  [[nodiscard]] std::string getKConvexity() { return K_convexity; };
  [[nodiscard]] std::string getBinomialConvexity() { return binomial_K_convexity; };
  [[nodiscard]] std::string getConvexity() { return convexity; };
  [[nodiscard]] Direction get_direction() const { return direction; };
  [[nodiscard]] WorkerState get_initial_state() const { return ini_state; };

  [[nodiscard]] std::vector<int> feasibleActions() const;
  [[nodiscard]] double immediateValue(WorkerState ini_state, int action, int overturn_num) const;
  [[nodiscard]] WorkerState stateTransition(WorkerState ini_state, int action,
                                            int overturn_num) const;
  double recursionForward(WorkerState ini_state);
  void recursionBackwardParallel();
  void computeStage(int t, int start_inventory, int end_inventory,
                    std::vector<std::unordered_map<WorkerState, double>> &values,
                    std::vector<std::unordered_map<WorkerState, double>> &policies);
  std::vector<double> solve(WorkerState ini_state);
  [[nodiscard]] std::vector<std::array<int, 2>> findsS() const;
  void simulatesS(WorkerState ini_state, const std::vector<std::array<int, 2>> &sS) const;
  std::vector<double> computeGy();
  std::vector<std::vector<double>> computeExpectGy(const std::vector<double> &Gy) const;
  [[nodiscard]] std::vector<std::vector<double>> getOptTable() const;

  std::pair<double, std::vector<std::array<double, 2>>> solveMip() const;

  bool checkKConvexity(const std::vector<double> &Gy);
  bool checkBinomialKConvexity(const std::vector<double> &Gy,
                               const std::vector<std::vector<double>> &expect_Gy);
  bool checkConvexity(const std::vector<double> &Gy);
};
#endif // WORKFORCE_PLAN_H
