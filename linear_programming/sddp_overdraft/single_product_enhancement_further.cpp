/*
 * Created by Zhen Chen on 2025/3/13.
 * Email: chen.zhen5526@gmail.com
 * Description:
 * without removing duplicate constraints, running time is 2.2s while 0.83s for
 * removing.
 *
 *
 * Skipping some backward computation is a heuristic, since the cut constraints
 * can affect the dual values of decision variables as well. For example,
 * when rhs of the inventory flow constraint is 1 or 21, although both positive,
 * they can have different duals since high inventory may result in salvage
 * values.
 *
 */
#include "single_product_enhancement_further.h"

std::array<double, 2> SingleProduct::solve() const {
  const std::vector<int> sampleNums(T, sampleNum);
  std::vector<std::vector<double>> sampleDetails(T);
  for (int t = 0; t < T; t++) {
    sampleDetails[t].resize(sampleNums[t]);
    sampleDetails[t] = generateSamplesPoisson(sampleNums[t], mean_demands[t]);
  }
  // sampleDetails = {{5, 15}, {5, 15}, {5, 15}};

  // 创建 Gurobi 环境与模型
  GRBEnv env(true); //  If empty=true, creates an empty environment.
  // Use GRBEnv::start to start the environment if true.
  env.set(GRB_IntParam_OutputFlag, 0);
  env.start();
  std::vector<GRBModel> models(T + 1, GRBModel(env));

  // // 创建 Gurobi 环境
  // auto env = GRBEnv();
  // // 模型数量
  // size_t numModels = T + 1;
  // // 创建一个模型数组
  // env.set(GRB_IntParam_OutputFlag, 0);
  // std::vector<GRBModel> models;
  // // 初始化模型数组
  // for (int i = 0; i < numModels; ++i) {
  //   models.emplace_back(env);
  // }

  // decision variables
  std::vector<GRBVar> q(T);
  std::vector<GRBVar> q_pre(T);
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
      models[t].update();
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
        models[t].addConstr(iniCash - unitVariOderCosts[t] * q[t] - W0[t] + W1[t] + W2[t] ==
                            overhead_costs[t]);
      else {
        models[t].addConstr(cash[t - 1] - unitVariOderCosts[t] * q[t] - W0[t] + W1[t] + W2[t] ==
                            overhead_costs[t]);
      }
      models[t].addConstr(theta[t] >= thetaInitialValue * (static_cast<double>(T) - t));
      models[t].update();
    }
    if (t == 0)
      models[t].setObjective(overhead_costs[0] + unitVariOderCosts[0] * q[0] + r2 * W2[0] +
                             r1 * W1[0] - r0 * W0[0] + theta[0]);
  }

  // double intercepts[iterNum][T][forwardNum];
  // double slopes3[iterNum][T][forwardNum];
  // double slopes2[iterNum][T][forwardNum];
  // double slopes1[iterNum][T][forwardNum];
  double qpreValues[iterNum][T][forwardNum];
  double qValues[iterNum][T][forwardNum];

  std::vector slopes1(iterNum, std::vector(T, std::vector<double>(forwardNum)));
  std::vector slopes2(iterNum, std::vector(T, std::vector<double>(forwardNum)));
  std::vector slopes3(iterNum, std::vector(T, std::vector<double>(forwardNum)));
  std::vector intercepts(iterNum, std::vector(T, std::vector<double>(forwardNum)));
  // std::vector qpreValues(iterNum,
  //                        std::vector(T, std::vector<double>(forwardNum)));
  // std::vector qValues(iterNum, std::vector(T,
  // std::vector<double>(forwardNum)));

  // double IForwardValues[iterNum][T][forwardNum];
  // double W0ForwardValues[iterNum][T][forwardNum];
  // double W1ForwardValues[iterNum][T][forwardNum];
  // double W2ForwardValues[iterNum][T][forwardNum];
  std::vector IForwardValues(iterNum, std::vector(T, std::vector<double>(forwardNum)));
  std::vector W0ForwardValues(iterNum, std::vector(T, std::vector<double>(forwardNum)));
  std::vector W1ForwardValues(iterNum, std::vector(T, std::vector<double>(forwardNum)));
  std::vector W2ForwardValues(iterNum, std::vector(T, std::vector<double>(forwardNum)));

  // no duplicate cuts during iteration
  std::vector<std::unordered_set<std::vector<double>, VectorHash>> cut_coefficients_cache(T);

  int iter = 0;
  while (iter < iterNum) {
    std::vector results_status(
        T - 1, std::vector<std::unordered_map<PairStatus, std::array<std::vector<double>, 2>>>(
                   forwardNum));

    // for checking inventory and W status
    std::vector<std::unordered_map<IStatus, std::array<std::vector<double>, 2>>>
        results_status_lastStage(forwardNum);

    auto scenarioPaths = generateScenarioPaths(forwardNum, sampleNums);

    // scenarioPaths = {{0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}, // change
    //                  {1, 1, 0}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1}};

    if (iter > 0) {

      if (iter == 1) { // remove the big M constraints at iteration 2
        int index = models[0].get(GRB_IntAttr_NumConstrs) - 1;
        models[0].remove(models[0].getConstr(index));
      }

      std::vector<double> thisCoefficients = {slopes1[iter - 1][0][0], slopes2[iter - 1][0][0],
                                              slopes3[iter - 1][0][0], intercepts[iter - 1][0][0]};

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
            }
            cut_coefficients_cache[t].emplace(finalCoefficient);
          }
          models[t].addConstr(theta[t] >=
                              finalCoefficient[0] * (I[t - 1] + q_pre[t - 1]) +
                                  finalCoefficient[1] *
                                      ((1 + r0) * W0[t] - (1 + r1) * W1[t] - (1 + r2) * W2[t]) +
                                  finalCoefficient[2] * q[t] + finalCoefficient[3]);
          models[t].update();
        }
      }

      // forward
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
          models[t].setObjective(overhead_costs[t] + unitVariOderCosts[t] * q[t] -
                                 prices[t - 1] * (demand - B[t - 1]) + r2 * W2[t] + r1 * W1[t] -
                                 r0 * W0[t] + theta[t]);

          models[t].getConstr(1).set(GRB_DoubleAttr_RHS, rhs2);
          models[t].getConstr(2).set(GRB_DoubleAttr_RHS, rhs3);
        } else {
          models[t].setObjective(-prices[t - 1] * (demand - B[t - 1]) -
                                 unitSalvageValue * I[t - 1]);
        }
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
    std::vector intercept_values_backward(T, std::vector<std::vector<double>>(forwardNum));
    std::vector slope1_values_backward(T, std::vector<std::vector<double>>(forwardNum));
    std::vector slope2_values_backward(T, std::vector<std::vector<double>>(forwardNum));
    std::vector slope3_values_backward(T, std::vector<std::vector<double>>(forwardNum));
    for (int t = 0; t < T; t++) {
      for (int n = 0; n < forwardNum; n++) {
        intercept_values_backward[t][n].resize(sampleNums[t]);
        slope1_values_backward[t][n].resize(sampleNums[t]);
        slope2_values_backward[t][n].resize(sampleNums[t]);
        slope3_values_backward[t][n].resize(sampleNums[t]);
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

      IStatus status_last_stage;
      PairStatus status;

      for (int n = 0; n < forwardNum; n++) {
        size_t S = sampleDetails[t - 1].size();
        double last_q = 0;
        double rhs2 = 0;
        int piNum = models[t].get(GRB_IntAttr_NumConstrs);
        std::vector<double> pi(piNum);
        std::vector<double> rhs(piNum);

        for (size_t s = 0; s < S; s++) {
          bool skip = false;
          auto demand = sampleDetails[t - 1][s];
          double rhs1 = t == 1
                            ? iniI - demand
                            : IForwardValues[iter][t - 2][n] + qpreValues[iter][t - 2][n] - demand;
          if (t == T) {
            if (status_last_stage = rhs1 > 0 ? IStatus::POSITIVE : IStatus::NEGATIVE;
                results_status_lastStage[n].contains(status_last_stage)) {
              skip = true;
              pi = results_status_lastStage[n][status_last_stage][0];
              rhs = results_status_lastStage[n][status_last_stage][1];
              rhs[0] = rhs1;
            }
            models[t].setObjective(-prices[t - 1] * (demand - B[t - 1]) -
                                   unitSalvageValue * I[t - 1]);
          }
          if (t < T) {
            rhs2 = prices[t - 1] * demand + (1 + r0) * W0ForwardValues[iter][t - 1][n] -
                   (1 + r1) * W1ForwardValues[iter][t - 1][n] -
                   (1 + r2) * W2ForwardValues[iter][t - 1][n];
            double rhs3 = qValues[iter][t - 1][n];
            if (s > 0) {
              double this_end_cash = rhs1 > 0 ? rhs2 : rhs2 + prices[t - 1] * rhs1;
              double this_W = this_end_cash - overhead_costs[t] - unitVariOderCosts[t] * last_q;
              status = checkPairStatus(rhs1, this_W, overdraftLimit);
              if (results_status[t - 1][n].contains(status)) {
                //                skip = true;
                skip = s % 3 != 0 ? true : false;
                pi = results_status[t - 1][n][status][0];
                rhs = results_status[t - 1][n][status][1];
                rhs[0] = rhs1;
                rhs[1] = rhs2;
              }
            }
            models[t].setObjective(overhead_costs[t] + unitVariOderCosts[t] * q[t] -
                                   prices[t - 1] * (demand - B[t - 1]) + r2 * W2[t] + r1 * W1[t] -
                                   r0 * W0[t] + theta[t]);
            models[t].getConstr(1).set(GRB_DoubleAttr_RHS, rhs2);
            models[t].getConstr(2).set(GRB_DoubleAttr_RHS, rhs3);
          }

          if (skip == false) {
            models[t].getConstr(0).set(GRB_DoubleAttr_RHS, rhs1);
            // optimize
            models[t].optimize();
            if (t < T)
              last_q = q[t].get(GRB_DoubleAttr_X);

            for (int p = 0; p < piNum; p++) {
              GRBConstr constraint = models[t].getConstr(p);
              pi[p] = constraint.get(GRB_DoubleAttr_Pi);
              rhs[p] = constraint.get(GRB_DoubleAttr_RHS);
            }

            if (t == T) {
              status_last_stage = rhs1 > 0 ? IStatus::POSITIVE : IStatus::NEGATIVE;
              results_status_lastStage[n][status_last_stage] = {pi, rhs};
            } else {
              double this_end_cash = rhs1 > 0 ? rhs2 : rhs2 + prices[t - 1] * rhs1;
              double this_W = this_end_cash - overhead_costs[t] - unitVariOderCosts[t] * last_q;
              status = checkPairStatus(rhs1, this_W, overdraftLimit);
              results_status[t - 1][n][status] = {pi, rhs};
            }

            // if (iter == 4 and t == 2 and n == 5 and s == 1) {
            //   models[t].write("iter" + std::to_string(iter + 1) + "_sub_" +
            //                 std::to_string(t) + "^" + std::to_string(n + 1) +
            //                 "_" + std::to_string(s + 1) + "back.lp");
            //
            // models[t].write("iter" + std::to_string(iter + 1) + "_sub_" +
            //                 std::to_string(t) + "^" + std::to_string(n + 1) +
            //                 "_" + std::to_string(s + 1) + "back.sol");
            // std::string filename = "iter" + std::to_string(iter + 1) +
            //                        "_sub_" + std::to_string(t) + "^" +
            //                        std::to_string(n + 1) + "_" +
            //                        std::to_string(s + 1) + ".txt";
            // std::ofstream outFile(filename); // 打开文件进行写入
            // outFile << "demand: " << demand << std::endl;
            // outFile << "pi:";
            // for (int i = 0; i < piNum; ++i) {
            //   outFile << pi[i] << " "; // 将数组中的每个元素写入文件
            // }
            // outFile << "\n";
            // outFile << "rhs:";
            // for (int i = 0; i < piNum; ++i) {
            //   outFile << rhs[i] << " "; // 将数组中的每个元素写入文件
            // }
            // outFile.close();
            // ;
            // }
          }

          if (t < T) {
            intercept_values_backward[t - 1][n][s] += -pi[0] * demand +
                                                      pi[1] * prices[t - 1] * demand -
                                                      prices[t - 1] * demand + overhead_costs[t];
          } else {
            intercept_values_backward[t - 1][n][s] += -pi[0] * demand - prices[t - 1] * demand;
          }
          for (size_t k = 3; k < piNum; k++)
            intercept_values_backward[t - 1][n][s] += pi[k] * rhs[k];
          slope1_values_backward[t - 1][n][s] = pi[0];
          if (t < T) {
            slope2_values_backward[t - 1][n][s] = pi[1];
            slope3_values_backward[t - 1][n][s] = pi[2];
          }
        }
        double avgIntercept;
        double avgSlope1;
        double avgSlope2;
        double avgSlope3;
        for (size_t s = 0; s < S; s++) {
          double sum = std::accumulate(intercept_values_backward[t - 1][n].begin(),
                                       intercept_values_backward[t - 1][n].end(), 0.0);
          avgIntercept = sum / static_cast<double>(S);
          sum = std::accumulate(slope1_values_backward[t - 1][n].begin(),
                                slope1_values_backward[t - 1][n].end(), 0.0);
          avgSlope1 = sum / static_cast<double>(S);
          sum = std::accumulate(slope2_values_backward[t - 1][n].begin(),
                                slope2_values_backward[t - 1][n].end(), 0.0);
          avgSlope2 = sum / static_cast<double>(S);
          sum = std::accumulate(slope3_values_backward[t - 1][n].begin(),
                                slope3_values_backward[t - 1][n].end(), 0.0);
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

  double finalValue = -models[0].get(GRB_DoubleAttr_ObjVal);
  double Q1 = qValues[iter - 1][0][0];
  return {finalValue, Q1};
}

int main() {
  const SingleProduct singleProduct;
  const auto start_time = std::chrono::high_resolution_clock::now();
  const auto result = singleProduct.solve();
  const auto end_time = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> diff = end_time - start_time;
  std::cout << "********************************************" << std::endl;
  std::cout << "cpu time is: " << diff.count() << " seconds" << std::endl;
  std::cout << "final expected cash balance is " << result[0] << std::endl;
  std::cout << "ordering Q in the first period is " << result[1] << std::endl;
  double optimal_value = 167.31;
  double gap = (result[0] - optimal_value) / optimal_value;
  std::cout << "gap is " << std::format("{: .2f}%", gap * 100) << std::endl;

  return 0;
}
