/*
 * Created by Zhen Chen on 2025/3/13.
 * Email: chen.zhen5526@gmail.com
 * Description:
 * optimal solution is 167.31;
 * without removing duplicate constraints, running time is 2.2s while 0.83s for
 * removing; with avoiding adding same constraints during iteration, running
 * time is 0.66s; with pre solving some vales of variables in forward computing,
 * running time is 0.48s;
 *
 */
#include "single_product_enhancement.h"

std::array<double, 2> SingleProduct::solve() const {
  const std::vector<int> sampleNums(T, sampleNum);
  std::vector<std::vector<double>> sampleDetails(T);
  for (int t = 0; t < T; t++) {
    sampleDetails[t].resize(sampleNums[t]);
    sampleDetails[t] = generateSamplesPoisson(sampleNums[t], mean_demands[t]);
  }
  // sampleDetails = {{5, 15}, {5, 15}, {5, 15}};

  // 创建 Gurobi 环境与模型
  GRBEnv env = GRBEnv(true);
  env.set(GRB_IntParam_OutputFlag, 0);
  env.start();
  std::vector<GRBModel> models(T + 1, GRBModel(env));

  // decision variables
  std::vector<GRBVar> q(T);
  std::vector<GRBVar> q_pre(T - 1);
  std::vector<GRBVar> theta(T);
  std::vector<GRBVar> I(T);
  std::vector<GRBVar> B(T);
  std::vector<GRBVar> cash(T);
  std::vector<GRBVar> W0(T);
  std::vector<GRBVar> W1(T);
  std::vector<GRBVar> W2(T);

  // build initial models for each stage
  for (int t = 0; t < T + 1; t++) {
    if (t > 0) {
      cash[t - 1] = models[t].addVar(-GRB_INFINITY, GRB_INFINITY, 0, GRB_CONTINUOUS,
                                     "cash_" + std::to_string(t));
      I[t - 1] = models[t].addVar(0, GRB_INFINITY, 0, GRB_CONTINUOUS, "I_" + std::to_string(t));
      B[t - 1] = models[t].addVar(0, GRB_INFINITY, 0, GRB_CONTINUOUS, "B_" + std::to_string(t));
      models[t].addConstr(I[t - 1] - B[t - 1] == 0);
      if (t < T) {
        models[t].addConstr(cash[t - 1] + prices[t - 1] * B[t - 1] == 0);
        q_pre[t - 1] =
            models[t].addVar(0, GRB_INFINITY, 0, GRB_CONTINUOUS, "q_pre_" + std::to_string(t + 1));
        models[t].addConstr(q_pre[t - 1] == 0);
      }
    }
    if (t < T) {
      q[t] = models[t].addVar(0, GRB_INFINITY, 0, GRB_CONTINUOUS, "q_" + std::to_string(t + 1));
      W0[t] = models[t].addVar(0, GRB_INFINITY, 0, GRB_CONTINUOUS, "W0_" + std::to_string(t + 1));
      W1[t] = models[t].addVar(0, GRB_INFINITY, 0, GRB_CONTINUOUS, "W1_" + std::to_string(t + 1));
      W2[t] = models[t].addVar(0, GRB_INFINITY, 0, GRB_CONTINUOUS, "W2_" + std::to_string(t + 1));
      theta[t] = models[t].addVar(-GRB_INFINITY, GRB_INFINITY, 0, GRB_CONTINUOUS,
                                  "theta_" + std::to_string(t + 2));
      models[t].addConstr(W1[t] <= overdraftLimit);
      if (t == 0)
        models[t].addConstr(iniCash - unit_vari_costs[t] * q[t] - W0[t] + W1[t] + W2[t] ==
                            overhead_costs[t]);
      else {
        models[t].addConstr(cash[t - 1] - unit_vari_costs[t] * q[t] - W0[t] + W1[t] + W2[t] ==
                            overhead_costs[t]);
      }
      models[t].addConstr(theta[t] >= thetaInitialValue * (static_cast<double>(T) - t));
    }
    if (t == 0)
      models[t].setObjective(overhead_costs[0] + unit_vari_costs[0] * q[0] + r2 * W2[0] +
                             r1 * W1[0] - r0 * W0[0] + theta[0]);
    models[t].update();
  }

  double intercepts[iterNum][T][forwardNum];
  double slopes3[iterNum][T][forwardNum];
  double slopes2[iterNum][T][forwardNum];
  double slopes1[iterNum][T][forwardNum];
  double qpreValues[iterNum][T][forwardNum];
  double qValues[iterNum][T][forwardNum];

  // no duplicate cuts during iteration
  std::vector<std::unordered_set<std::vector<double>, VectorHash>> cut_coefficients_cache(T);

  double IForwardValues[iterNum][T][forwardNum];
  //  double BForwardValues[iterNum][T][forwardNum];
  //  double cashForwardValues[iterNum][T][forwardNum];
  double W0ForwardValues[iterNum][T][forwardNum];
  double W1ForwardValues[iterNum][T][forwardNum];
  double W2ForwardValues[iterNum][T][forwardNum];

  int iter = 0;
  while (iter < iterNum) {
    auto scenarioPaths = generateScenarioPaths(forwardNum, sampleNums);
    // scenarioPaths = {{0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 1, 1},
    //                  {1, 0, 0}, {1, 0, 1}, {1, 1, 0}, {1, 1, 1}};

    if (iter > 0) {
      if (iter == 1) { // remove the big M constraints at iteration 2
        int index = models[0].get(GRB_IntAttr_NumConstrs) - 1;
        models[0].remove(models[0].getConstr(index));
      }

      std::vector<double> thisCoefficients = {slopes1[iter - 1][0][0], slopes2[iter - 1][0][0],
                                              slopes3[iter - 1][0][0], intercepts[iter - 1][0][0]};

      // models[0].addConstr(
      //     theta[0] >=
      //     thisCoefficients[0] * iniI +
      //         thisCoefficients[1] *
      //             ((1 + r0) * W0[0] - (1 + r1) * W1[0] - (1 + r2) * W2[0]) +
      //         thisCoefficients[2] * q[0] + thisCoefficients[3]);
      // models[0].update();
      if (!cut_coefficients_cache.empty()) {
        if (!cut_coefficients_cache[0].contains(thisCoefficients) ||
            cut_coefficients_cache[0].empty()) {
          models[0].addConstr(theta[0] >=
                              thisCoefficients[0] * iniI +
                                  thisCoefficients[1] *
                                      ((1 + r0) * W0[0] - (1 + r1) * W1[0] - (1 + r2) * W2[0]) +
                                  thisCoefficients[2] * q[0] + thisCoefficients[3]);
          models[0].update();

          cut_coefficients_cache[0].emplace(thisCoefficients);
        }
      }
    }
    models[0].optimize();

    // int optimstatus = models[0].get(GRB_IntAttr_Status);
    // std::cout << "optimization status: " << optimstatus << std::endl;
    // models[0].write("iter_" + std::to_string(iter + 1) + ".lp");
    // models[0].write("iter_" + std::to_string(iter + 1) + ".sol");

    for (int n = 0; n < forwardNum; n++) {
      qValues[iter][0][n] = q[0].get(GRB_DoubleAttr_X);
      W0ForwardValues[iter][0][n] = W0[0].get(GRB_DoubleAttr_X);
      W1ForwardValues[iter][0][n] = W1[0].get(GRB_DoubleAttr_X);
      W2ForwardValues[iter][0][n] = W2[0].get(GRB_DoubleAttr_X);
    }

    // forward
    for (int t = 1; t < T + 1; t++) {

      if (iter == 1 and t < T) { // remove the big M constraints at iteration 2
        int index = models[t].get(GRB_IntAttr_NumConstrs) - 1;
        models[t].remove(models[t].getConstr(index));
      }

      if (iter > 0 && t < T) {
        std::vector<std::vector<double>> cutCoefficients(forwardNum, std::vector<double>(4, 0));
        for (int n = 0; n < forwardNum; n++) {
          cutCoefficients[n][0] = slopes1[iter - 1][t][n];
          cutCoefficients[n][1] = slopes2[iter - 1][t][n];
          cutCoefficients[n][2] = slopes3[iter - 1][t][n];
          cutCoefficients[n][3] = intercepts[iter - 1][t][n];
        };

        for (auto finalCoefficients = removeDuplicateRows(cutCoefficients); // cutCoefficients
             auto &finalCoefficient : finalCoefficients) {
          if (!cut_coefficients_cache.empty()) {
            if (cut_coefficients_cache[t].contains(finalCoefficient)) {
              continue;
            } else {
              cut_coefficients_cache[t].emplace(finalCoefficient);
            }
          }
          models[t].addConstr(theta[t] >=
                              finalCoefficient[0] * (I[t - 1] + q_pre[t - 1]) +
                                  finalCoefficient[1] *
                                      ((1 + r0) * W0[t] - (1 + r1) * W1[t] - (1 + r2) * W2[t]) +
                                  finalCoefficient[2] * q[t] + finalCoefficient[3]);
        }
      }

      for (int n = 0; n < forwardNum; n++) {
        double rhs2 = 0;
        int index = scenarioPaths[n][t - 1];
        double demand = sampleDetails[t - 1][index];
        double rhs1 = t == 1 ? iniI - demand
                             : IForwardValues[iter][t - 2][n] + qpreValues[iter][t - 2][n] - demand;
        if (t < T) {
          rhs2 = prices[t - 1] * demand + (1 + r0) * W0ForwardValues[iter][t - 1][n] -
                 (1 + r1) * W1ForwardValues[iter][t - 1][n] -
                 (1 + r2) * W2ForwardValues[iter][t - 1][n];
          double rhs3 = qValues[iter][t - 1][n];
          models[t].setObjective(overhead_costs[t] + unit_vari_costs[t] * q[t] -
                                 prices[t - 1] * (demand - B[t - 1]) + r2 * W2[t] + r1 * W1[t] -
                                 r0 * W0[t] + theta[t]);
          models[t].getConstr(1).set(GRB_DoubleAttr_RHS, rhs2);
          models[t].getConstr(2).set(GRB_DoubleAttr_RHS, rhs3);
        } else
          models[t].setObjective(-prices[t - 1] * (demand - B[t - 1]) -
                                 unit_salvage_value * I[t - 1]);
        models[t].getConstr(0).set(GRB_DoubleAttr_RHS, rhs1);

        // set lb and up for some variables
        double this_I_value = rhs1 > 0 ? rhs1 : 0;
        double this_B_value = rhs1 < 0 ? -rhs1 : 0;
        I[t - 1].set(GRB_DoubleAttr_LB, this_I_value);
        I[t - 1].set(GRB_DoubleAttr_UB, this_I_value);
        B[t - 1].set(GRB_DoubleAttr_LB, this_B_value);
        B[t - 1].set(GRB_DoubleAttr_UB, this_B_value);
        if (t < T) {
          double this_end_cash = rhs2 - prices[t - 1] * this_B_value;
          cash[t - 1].set(GRB_DoubleAttr_LB, this_end_cash);
          cash[t - 1].set(GRB_DoubleAttr_UB, this_end_cash);
        }

        // optimize
        models[t].optimize();
        if (models[t].get(GRB_IntAttr_Status) != 2) {
          models[t].write("iter" + std::to_string(iter + 1) + "_sub_" + std::to_string(t) + "^" +
                          std::to_string(n + 1) + ".lp");

          std::cout << "optimizing status " << models[t].get(GRB_IntAttr_Status) << std::endl;
        }

        try {
          IForwardValues[iter][t - 1][n] = I[t - 1].get(GRB_DoubleAttr_X);
        } catch (...) {
          models[t].write("iter" + std::to_string(iter + 1) + "_sub_" + std::to_string(t) + "^" +
                          std::to_string(n + 1) + ".lp");

          std::cout << "optimizing status " << models[t].get(GRB_IntAttr_Status) << std::endl;
          ;
        }

        if (t < T) {
          qValues[iter][t][n] = q[t].get(GRB_DoubleAttr_X);
          qpreValues[iter][t - 1][n] = q_pre[t - 1].get(GRB_DoubleAttr_X);
          W0ForwardValues[iter][t][n] = W0[t].get(GRB_DoubleAttr_X);
          W1ForwardValues[iter][t][n] = W1[t].get(GRB_DoubleAttr_X);
          W2ForwardValues[iter][t][n] = W2[t].get(GRB_DoubleAttr_X);
        }
      }
    }

    // backward
    std::vector interceptValues(T, std::vector<std::vector<double>>(forwardNum));
    std::vector slope1Values(T, std::vector<std::vector<double>>(forwardNum));
    std::vector slope2Values(T, std::vector<std::vector<double>>(forwardNum));
    std::vector slope3Values(T, std::vector<std::vector<double>>(forwardNum));
    for (int t = 0; t < T; t++) {
      for (int n = 0; n < forwardNum; n++) {
        interceptValues[t][n].resize(sampleNums[t]);
        slope1Values[t][n].resize(sampleNums[t]);
        slope2Values[t][n].resize(sampleNums[t]);
        slope3Values[t][n].resize(sampleNums[t]);
      }
    }
    for (size_t t = T; t > 0; t--) {
      // de set lb and up for some variables
      I[t - 1].set(GRB_DoubleAttr_LB, 0.0);
      I[t - 1].set(GRB_DoubleAttr_UB, GRB_INFINITY);
      B[t - 1].set(GRB_DoubleAttr_LB, 0.0);
      B[t - 1].set(GRB_DoubleAttr_UB, GRB_INFINITY);
      if (t < T) {
        cash[t - 1].set(GRB_DoubleAttr_LB, -GRB_INFINITY);
        cash[t - 1].set(GRB_DoubleAttr_UB, GRB_INFINITY);
      }

      for (int n = 0; n < forwardNum; n++) {
        size_t S = sampleDetails[t - 1].size();
        for (size_t s = 0; s < S; s++) {
          auto demand = sampleDetails[t - 1][s];
          double rhs1 = t == 1
                            ? iniI - demand
                            : IForwardValues[iter][t - 2][n] + qpreValues[iter][t - 2][n] - demand;
          if (t < T) {
            double rhs2 = prices[t - 1] * demand + (1 + r0) * W0ForwardValues[iter][t - 1][n] -
                          (1 + r1) * W1ForwardValues[iter][t - 1][n] -
                          (1 + r2) * W2ForwardValues[iter][t - 1][n];
            double rhs3 = qValues[iter][t - 1][n];
            models[t].setObjective(overhead_costs[t] + unit_vari_costs[t] * q[t] -
                                   prices[t - 1] * (demand - B[t - 1]) + r2 * W2[t] + r1 * W1[t] -
                                   r0 * W0[t] + theta[t]);
            models[t].getConstr(1).set(GRB_DoubleAttr_RHS, rhs2);
            models[t].getConstr(2).set(GRB_DoubleAttr_RHS, rhs3);
          } else
            models[t].setObjective(-prices[t - 1] * (demand - B[t - 1]) -
                                   unit_salvage_value * I[t - 1]);
          models[t].getConstr(0).set(GRB_DoubleAttr_RHS, rhs1);

          // optimize
          models[t].optimize();
          if (models[t].get(GRB_IntAttr_Status) != 2) {
            std::cout << models[t].get(GRB_IntAttr_Status) << "\n";
            models[t].write("iter_" + std::to_string(iter + 1) + "_sub_" + std::to_string(t) + "^" +
                            std::to_string(n + 1) + ".lp");
          }
          // if (iter == 3 and t == 1) {
          //   models[t].write("iter_" + std::to_string(iter + 1) + "_sub_" +
          //                   std::to_string(t) + "^" + std::to_string(n + 1) +
          //                   ".lp");
          //   void();
          // }

          int piNum = models[t].get(GRB_IntAttr_NumConstrs);
          double pi[piNum];
          double rhs[piNum];
          for (int p = 0; p < piNum; p++) {
            GRBConstr constraint = models[t].getConstr(p);
            pi[p] = constraint.get(GRB_DoubleAttr_Pi);
            rhs[p] = constraint.get(GRB_DoubleAttr_RHS);
          }
          if (t < T) {
            interceptValues[t - 1][n][s] += -pi[0] * demand + pi[1] * prices[t - 1] * demand -
                                            prices[t - 1] * demand + overhead_costs[t];
          } else {
            interceptValues[t - 1][n][s] += -pi[0] * demand - prices[t - 1] * demand;
          }
          for (size_t k = 3; k < piNum; k++) {
            interceptValues[t - 1][n][s] += pi[k] * rhs[k];
          }
          slope1Values[t - 1][n][s] = pi[0];
          if (t < T) {
            slope2Values[t - 1][n][s] = pi[1];
            slope3Values[t - 1][n][s] = pi[2];
          }
        }
        double avgIntercept;
        double avgSlope1;
        double avgSlope2;
        double avgSlope3;
        for (size_t s = 0; s < S; s++) {
          double sum = std::accumulate(interceptValues[t - 1][n].begin(),
                                       interceptValues[t - 1][n].end(), 0.0);
          avgIntercept = sum / static_cast<double>(S);
          sum = std::accumulate(slope1Values[t - 1][n].begin(), slope1Values[t - 1][n].end(), 0.0);
          avgSlope1 = sum / static_cast<double>(S);
          sum = std::accumulate(slope2Values[t - 1][n].begin(), slope2Values[t - 1][n].end(), 0.0);
          avgSlope2 = sum / static_cast<double>(S);
          sum = std::accumulate(slope3Values[t - 1][n].begin(), slope3Values[t - 1][n].end(), 0.0);
          avgSlope3 = sum / static_cast<double>(S);
        }
        slopes1[iter][t - 1][n] = avgSlope1;
        slopes2[iter][t - 1][n] = avgSlope2;
        slopes3[iter][t - 1][n] = avgSlope3;
        intercepts[iter][t - 1][n] = avgIntercept;
      }
    }
    // std::cout << "iteration " << iter << ", objective is " << std::fixed
    //           << std::setprecision(2) <<
    //           -models[0].get(GRB_DoubleAttr_ObjVal)
    //           << std::endl;
    iter = iter + 1;
  }

  std::cout << "********************************************" << std::endl;
  double finalValue = -models[0].get(GRB_DoubleAttr_ObjVal);
  double Q1 = qValues[iter - 1][0][0];

  std::cout << "after " << iterNum << " iterations: " << std::endl;
  std::cout << "final expected cash balance is " << finalValue << std::endl;
  std::cout << "ordering Q in the first period is " << Q1 << std::endl;

  return {finalValue, Q1};
}

int main() {
  auto singleProduct = SingleProduct();
  const auto start_time = std::chrono::high_resolution_clock::now();
  double finalValue = singleProduct.solve()[0];
  const auto end_time = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> diff = end_time - start_time;
  std::cout << "cpu time is: " << diff.count() << " seconds" << std::endl;
  double optimal_value = 167.31;
  double gap = (finalValue - optimal_value) / optimal_value;
  std::cout << "gap is " << std::format("{: .2f}%", gap * 100) << std::endl;
  return 0;
}
