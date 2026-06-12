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

#include <chrono>
#include <numeric>

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
    if (t < T) {
      q[t] = models[t].addVar(0, GRB_INFINITY, 0, GRB_CONTINUOUS, "q_" + std::to_string(t + 1));
      theta[t] = models[t].addVar(theta_lb, GRB_INFINITY, 0, GRB_CONTINUOUS,
                                  "theta_" + std::to_string(t + 2));
    }
    if (t > 0) {
      I[t - 1] = models[t].addVar(0, GRB_INFINITY, 0, GRB_CONTINUOUS, "I_" + std::to_string(t));
      B[t - 1] = models[t].addVar(0, GRB_INFINITY, 0, GRB_CONTINUOUS, "B_" + std::to_string(t));
      models[t].addConstr(I[t - 1] - B[t - 1] == 0);
      models[t].update(); // should update the model every time we add the contraint into the model
    }

    if (t == 0)
      models[t].setObjective(unit_vari_costs[0] * q[0] + theta[0]);
    else if (t == T)
      models[t].setObjective(unit_holding_costs[T - 1] * I[T - 1] +
                             unit_backorder_costs[T - 1] * B[T - 1]);
    else
      models[t].setObjective(unit_holding_costs[t - 1] * I[t - 1] +
                             unit_backorder_costs[t - 1] * B[t - 1] + unit_vari_costs[t] * q[t] +
                             theta[t]);
  }

  std::vector intercepts(iter_num, std::vector(T, std::vector<double>(forward_num, 0.0)));
  std::vector slopes(iter_num, std::vector(T, std::vector<double>(forward_num, 0.0)));
  std::vector q_values(iter_num, std::vector(T, std::vector<double>(forward_num, 0.0)));
  std::vector I_forward_values(iter_num, std::vector(T, std::vector<double>(forward_num, 0.0)));
  std::vector B_forward_values(iter_num, std::vector(T, std::vector<double>(forward_num, 0.0)));

  int iter = 0;
  while (iter < iter_num) {
    auto scenario_paths = generateScenarioPaths(forward_num, sample_nums);

    if (iter > 0) {
      models[0].addConstr(theta[0] >= slopes[iter - 1][0][0] * q[0] + intercepts[iter - 1][0][0]);
      models[0].update();
    }
    models[0].optimize();
    // models[0].write("iter_" + std::to_string(iter + 1) + ".lp");

    for (int n = 0; n < forward_num; n++) {
      q_values[iter][0][n] = q[0].get(GRB_DoubleAttr_X);
    }

    // forward
    for (int t = 1; t < T + 1; t++) {
      if (iter > 0 && t < T) {
        std::vector cut_coefficients(forward_num, std::vector<double>(2, 0));
        for (int n = 0; n < forward_num; n++) {
          cut_coefficients[n][0] = slopes[iter - 1][t][n];
          cut_coefficients[n][1] = intercepts[iter - 1][t][n];
        };

        // without enhancement
        for (auto &coefficient : cut_coefficients) {
          models[t].addConstr(theta[t] >=
                              coefficient[0] * (I[t - 1] - B[t - 1] + q[t]) + coefficient[1]);
        }
      }

      for (int n = 0; n < forward_num; n++) {
        int index = scenario_paths[n][t - 1];
        double demand = sample_details[t - 1][index];
        if (t < T)
          models[t].setObjective(unit_vari_costs[t] * q[t] +
                                 unit_backorder_costs[t - 1] * B[t - 1] +
                                 unit_holding_costs[t - 1] * I[t - 1] + theta[t]);
        else {
          models[t].setObjective(unit_backorder_costs[t - 1] * B[t - 1] +
                                 unit_holding_costs[t - 1] * I[t - 1]);
        }

        double rhs = t == 1 ? ini_I - demand + q_values[iter][t - 1][n]
                            : I_forward_values[iter][t - 2][n] - B_forward_values[iter][t - 2][n] +
                                  q_values[iter][t - 1][n] - demand;

        models[t].getConstr(0).set(GRB_DoubleAttr_RHS, rhs);
        models[t].update();
        models[t].optimize();

        if (models[t].get(GRB_IntAttr_SolCount) == 1) {
          I_forward_values[iter][t - 1][n] = I[t - 1].get(GRB_DoubleAttr_X);
          B_forward_values[iter][t - 1][n] = B[t - 1].get(GRB_DoubleAttr_X);
          if (t < T)
            q_values[iter][t][n] = q[t].get(GRB_DoubleAttr_X);
        }
      }
    }

    // backward
    std::vector intercepts_detail(T, std::vector<std::vector<double>>(forward_num));
    std::vector slopes_detail(T, std::vector<std::vector<double>>(forward_num));
    for (int t = 0; t < T; t++) {
      for (int n = 0; n < forward_num; n++) {
        intercepts_detail[t][n].resize(sample_nums[t]);
        slopes_detail[t][n].resize(sample_nums[t]);
      }
    }

    for (size_t t = T; t > 0; t--) {
      for (int n = 0; n < forward_num; n++) {
        size_t S = sample_details[t - 1].size();
        for (size_t s = 0; s < S; s++) {
          auto demand = sample_details[t - 1][s];
          double rhs = t == 1
                           ? ini_I - demand + q_values[iter][t - 1][n]
                           : I_forward_values[iter][t - 2][n] - B_forward_values[iter][t - 2][n] +
                                 q_values[iter][t - 1][n] - demand;
          models[t].getConstr(0).set(GRB_DoubleAttr_RHS, rhs);
          models[t].update();
          models[t].optimize();

          int pi_num = models[t].get(GRB_IntAttr_NumConstrs);
          std::vector<double> pi(pi_num);
          std::vector<double> rhs_detail(pi_num);
          for (int p = 0; p < pi_num; p++) {
            GRBConstr constraint = models[t].getConstr(p);
            pi[p] = constraint.get(GRB_DoubleAttr_Pi);
            rhs_detail[p] = constraint.get(GRB_DoubleAttr_RHS);
          }
          intercepts_detail[t - 1][n][s] = -pi[0] * demand;
          for (size_t k = 1; k < pi_num; k++)
            intercepts_detail[t - 1][n][s] += pi[k] * rhs_detail[k];
          slopes_detail[t - 1][n][s] = pi[0];
        }

        double avg_intercept;
        double avg_slope;
        for (size_t s = 0; s < S; s++) {
          double sum = std::accumulate(intercepts_detail[t - 1][n].begin(),
                                       intercepts_detail[t - 1][n].end(), 0.0);
          avg_intercept = sum / static_cast<double>(S);
          sum =
              std::accumulate(slopes_detail[t - 1][n].begin(), slopes_detail[t - 1][n].end(), 0.0);
          avg_slope = sum / static_cast<double>(S);
        }
        slopes[iter][t - 1][n] = avg_slope;
        intercepts[iter][t - 1][n] = avg_intercept;
      }
    }
    std::cout << "iter " << iter + 1 << " value is " << models[0].get(GRB_DoubleAttr_ObjVal)
              << std::endl;
    iter++;
  }
  double final_value = models[0].get(GRB_DoubleAttr_ObjVal);
  double Q1 = q_values[iter - 1][0][0];
  return {final_value, Q1};
}

int main() {
  const Newsvendor single_product;
  const auto start_time = std::chrono::high_resolution_clock::now();
  const auto result = single_product.solve();
  const auto end_time = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> diff = end_time - start_time;
  std::cout << "********************************************" << std::endl;
  std::cout << "cpu time is: " << diff.count() << " seconds" << std::endl;
  std::cout << "final expected cost is " << result[0] << std::endl;
  std::cout << "ordering Q in the first period is " << result[1] << std::endl;
  return 0;
}