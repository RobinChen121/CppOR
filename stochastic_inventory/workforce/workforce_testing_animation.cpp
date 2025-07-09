/*
 * Created by Zhen Chen on 2025/6/17.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */
#include "../../utils/draw_graph.h"
#include "workforce_plan.h"

int main() {
  int num = 10;
  std::vector<int> fix_costs(num);
  for (int i = 0; i < num; i++) {
    fix_costs[i] = (i + 1) * 50;
  }
  std::vector<int> salaries(num);
  for (int i = 0; i < num; i++) {
    salaries[i] = (i + 1) * 5;
  }
  std::vector<int> penalties(num);
  for (int i = 0; i < num; i++) {
    penalties[i] = (i + 1) * 10;
  }

  std::vector<int> min_workers(num);
  for (int i = 0; i < num; i++) {
    min_workers[i] = (i + 1) * 10;
  }
  num = 9;
  std::vector<double> turnover_rate(num);
  for (int i = 0; i < num; i++) {
    turnover_rate[i] = (i + 1) * 0.1;
  }

  std::vector<std::vector<std::array<double, 2>>> Gys(num);
  // std::vector<std::array<int, 2>> arr_ss(num);
  std::vector<std::string> arr_str(num);
  std::vector<std::string> parameter(num);
  std::vector<double> parameter_values(num);
  std::vector<std::string> kconvexity(num);
  std::vector<std::string> binomial_kconvexity(num);
  std::vector<std::string> convexity(num);
  for (int i = 0; i < num; i++) {
    auto problem = WorkforcePlan();

    problem.set_fix_cost(fix_costs[i]);
    // problem.set_salary(salaries[i]);
    // problem.set_penalty(penalties[i]);
    // problem.set_min_workers(min_workers[i]);
    // problem.set_turnover_rate(turnover_rate[i]);

    Gys[i] = problem.computeGy();
    // arr_ss[i] = problem.findsS()[0];
    parameter[i] = problem.get_varied_parameter();
    problem.checkKConvexity(Gys[i]);
    problem.checkBinomialKConvexity(Gys[i]);
    problem.checkConvexity(Gys[i]);
    kconvexity[i] = problem.getKConvexity();
    binomial_kconvexity[i] = problem.getBinomialConvexity();
    convexity[i] = problem.getConvexity();
  }

  drawGyAnimation(Gys, parameter, kconvexity, binomial_kconvexity, convexity);
  return 0;
}