/*
 * Created by Zhen Chen on 2026/8/31.
 * Email: chen.zhen5526@gmail.com
 * Description: use a 1-D vector to store the value function and policy function, which is faster
 * than using unordered_map.
 *
 *
 */

#ifndef WORKFORCE_WORKFORCE_PLAN_NEW_H
#define WORKFORCE_WORKFORCE_PLAN_NEW_H

#include "../../utils/pmf.h"
#include "worker_state.h"

#include <unordered_map>
#include <vector>

class WorkforcePlanNew {
  std::vector<double> turnover_rates = {0.1, 0.3, 0.5, 0.1, 0.3, 0.5, 0.1, 0.3};
  int T = turnover_rates.size();

  int initial_workers = 0;
  // 类初始化 {} 更安全，防止类属性窄化，例如从 double 到 int 这样的精度丢失
  WorkerState ini_state = WorkerState{1, initial_workers};
  double fix_hire_cost = 4000.0;
  double unit_vari_cost = 0.0;
  double salary = 2000.0;
  double unit_penalty = 3000.0;
  // 初始化给定默认值时就可以使用已声明变量的值
  std::vector<int> min_workers = std::vector<int>(T, 50);

  int max_hire_num = 500;
  int max_worker_num = 500;
  int piece_segment = 5;
  int state_num = max_worker_num + 1; // number of possible worker states, from 0 to max_worker_num

  // for DP using map
  std::unordered_map<WorkerState, int> cache_actions;
  std::unordered_map<WorkerState, double> cache_values;

  // for DP using 1-D vector
  // 类中 vector 初始化需要用花括号 {}，而不是圆括号 ()，否则会被解释为函数声明
  std::vector<double> value{
      std::vector<double>((T + 1) * state_num, 0.0)}; // 1-D vector to store the value function
  std::vector<int> policy{
      std::vector<int>(T * state_num, 0)}; // 1-D vector to store the policy function

  std::vector<std::vector<std::vector<double>>> pmf;

public:
  WorkforcePlanNew() { pmf = PMF::getPMFBinomial(max_worker_num, turnover_rates); };

  // 建议将所有单参数构造函数（或带有默认参数的构造函数）默认声明为 explicit
  // 否则下面两种情况会导致隐式转换
  // ExplicitVector ev1 = 10; // ExplicitVector 是一个单参数构造的类
  // printExplicitVector(20) // 这个函数的参数是一个 ExplicitVector 对象
  // explicit 禁止隐式转换和隐式初始化
  explicit WorkforcePlanNew(const std::vector<std::vector<std::vector<double>>> &pmf) : pmf(pmf) {};

  explicit WorkforcePlanNew(const std::vector<double> &turnover_rate, const double fix_hire_cost,
                            const double salary, const double unit_penalty,
                            const std::vector<int> &min_workers)
      : turnover_rates(turnover_rate), fix_hire_cost(fix_hire_cost), salary(salary),
        unit_penalty(unit_penalty), min_workers(min_workers) {};
};

#endif // WORKFORCE_WORKFORCE_PLAN_NEW_H
