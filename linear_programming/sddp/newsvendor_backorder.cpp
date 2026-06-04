/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 04/06/2026, 19:03
 * Description:
 *
 */

#include "newsvendor_backorder.h"
#include "../../utils/sampling.h"
#include "gurobi_c++.h"

std::array<double, 2> Newsvendor::solve() const {
  const std::vector sample_nums(T, sample_num);
  std::vector<std::vector<double>> sample_details(T);
  for (int t = 0; t < T; t++) {
    sample_details[t].resize(sample_nums[t]);
    sample_details[t] = generateSamplesPoisson(sample_nums[t], mean_demands[t]);
  }

  // gurobi environments and model
  auto env = GRBEnv(); //  If empty=true, creates an empty environment.
  // Use GRBEnv::start to start the environment if true.
  env.set(GRB_IntParam_OutputFlag, 0);
  auto model = GRBModel(env);
  // one model for each stage
  std::vector models(T + 1, GRBModel(env));

  // decision variables
  std::vector<GRBVar> q(T);
  std::vector<GRBVar> I(T);
  std::vector<GRBVar> B(T);
  std::vector<GRBVar> theta(T); // previous is T - 1, which causing the error
                                // build initial models for each stage
  for (int t = 0; t < T + 1; t++) {
    if (t == 0)
      models[t].setObjective(unit_vari_costs[0] * q[0] + theta[0]);
    else if (t == T)
      models[t].setObjective(unit_holding_costs[T] * I[T] + unit_backorder_costs[T] * B[T]);
    else
      models[t].setObjective(unit_holding_costs[t - 1] * I[t - 1] +
                             unit_backorder_costs[t - 1] * B[t - 1] + unit_vari_costs[0] * q[t] +
                             theta[t]);
    if (t < T) {
      q[t] = models[t].addVar(0, GRB_INFINITY, 0, GRB_CONTINUOUS, "q_" + std::to_string(t + 1));
      theta[t] = models[t].addVar(theta_lb, GRB_INFINITY, 0, GRB_CONTINUOUS,
                                  "theta_" + std::to_string(t + 2));
    }
    if (t > 0) {
      I[t - 1] = models[t].addVar(0, GRB_INFINITY, 0, GRB_CONTINUOUS, "I_" + std::to_string(t));
      B[t - 1] = models[t].addVar(0, GRB_INFINITY, 0, GRB_CONTINUOUS, "B_" + std::to_string(t));
      models[t].addConstr(I[t - 1] - B[t - 1] == 0);
    }
  }

  double intercepts[iter_num][T][forward_num];
  double slopes[iter_num][T][forward_num];
  double q_values[iter_num][T][forward_num];
  double I_forward_values[iter_num][T][forward_num];

  int iter = 0;
  while (iter < iter_num) {
    auto scenario_paths = generateScenarioPaths(forward_num, sample_nums);
  }
  return {};
}

int main() { return 0; }