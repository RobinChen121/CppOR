/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 11/06/2026, 19:32
 * Description: there are bugs in those codes. Highs C++ is not convenient to use.
 *
 */
#include "Highs.h"
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

using namespace std;

// ================= 工具函数 =================
default_random_engine gen(12345);

int sample_poisson(double mean) {
  poisson_distribution<int> dist(mean);
  return dist(gen);
}

double mean_val(const vector<double> &v) { return accumulate(v.begin(), v.end(), 0.0) / v.size(); }

// ================= 主程序 =================
int main() {

  // ===== 参数 =====
  vector<double> mean_demands = {10, 20, 10, 20, 10, 20, 10, 20};
  int T = mean_demands.size();

  double ini_I = 0.0;

  vector<double> c(T, 1.0);
  vector<double> h(T, 2.0);
  vector<double> b(T, 10.0);

  int iter_num = 30;
  int sample_num = 30;
  int forward_num = 1;

  // ===== 预生成样本 =====
  vector<vector<int>> sample_details(T, vector<int>(sample_num));

  for (int t = 0; t < T; t++) {
    for (int s = 0; s < sample_num; s++) {
      sample_details[t][s] = sample_poisson(mean_demands[t]);
    }
  }

  // ===== 建模 =====
  vector<Highs> models(T + 1);
  struct Vars {
    int q = -1, I = -1, B = -1, theta = -1;
  };
  vector<Vars> vars(T + 1);

  vector<int> balance_row(T + 1, -1);

  // ===== 初始化 =====
  for (int t = 0; t <= T; t++) {
    models[t].setOptionValue("output_flag", false);
  }

  // ===== 构建变量 & 约束 =====
  for (int t = 0; t <= T; t++) {
    Highs &m = models[t];

    if (t < T) {
      vars[t].q = m.getLp().num_col_;
      m.addCol(0.0, 0.0, kHighsInf, 0, NULL, NULL);

      vars[t].theta = m.getLp().num_col_;
      m.addCol(0.0, 0.0, kHighsInf, 0, NULL, NULL);
    }

    if (t > 0) {
      vars[t].I = m.getLp().num_col_;
      m.addCol(0.0, 0.0, kHighsInf, 0, NULL, NULL);

      vars[t].B = m.getLp().num_col_;
      m.addCol(0.0, 0.0, kHighsInf, 0, NULL, NULL);

      int ind[2] = {vars[t].I, vars[t].B};
      double val[2] = {1.0, -1.0};

      balance_row[t] = m.getLp().num_row_;
      m.addRow(0.0, 0.0, 2, ind, val);
    }
  }

  // ===== 存储 =====
  vector<vector<vector<double>>> slopes(
      iter_num, vector<vector<double>>(T, vector<double>(forward_num, 0.0)));

  vector<vector<vector<double>>> intercepts(
      iter_num, vector<vector<double>>(T, vector<double>(forward_num, 0.0)));

  vector<vector<vector<double>>> q_values(
      iter_num, vector<vector<double>>(T, vector<double>(forward_num, 0.0)));

  vector<vector<vector<double>>> I_values(
      iter_num, vector<vector<double>>(T, vector<double>(forward_num, 0.0)));

  vector<vector<vector<double>>> B_values(
      iter_num, vector<vector<double>>(T, vector<double>(forward_num, 0.0)));

  // ================= SDDP =================
  for (int iter = 0; iter < iter_num; iter++) {

    // ===== 生成 scenario paths =====
    vector<vector<int>> scenario_paths(forward_num, vector<int>(T));

    for (int n = 0; n < forward_num; n++) {
      for (int t = 0; t < T; t++) {
        scenario_paths[n][t] = rand() % sample_num;
      }
    }

    // ===== Stage 1 =====
    {
      Highs &m = models[0];

      vector<double> cost(m.getLp().num_col_, 0.0);
      cost[vars[0].q] = c[0];
      cost[vars[0].theta] = 1.0;

      m.changeColsCost(0, m.getLp().num_col_ - 1, cost.data());

      m.run();
      m.writeModel("stage1.lp");

      auto sol = m.getSolution();
      q_values[iter][0][0] = sol.col_value[vars[0].q];
    }

    // ===== FORWARD =====
    for (int t = 1; t <= T; t++) {
      Highs &m = models[t];

      for (int n = 0; n < forward_num; n++) {

        double demand = sample_details[t - 1][scenario_paths[n][t - 1]];

        double rhs;
        if (t == 1)
          rhs = ini_I - demand + q_values[iter][t - 1][n];
        else
          rhs = I_values[iter][t - 2][n] - B_values[iter][t - 2][n] + q_values[iter][t - 1][n] -
                demand;

        m.changeRowBounds(balance_row[t], rhs, rhs);

        vector<double> cost(m.getLp().num_col_, 0.0);

        if (t < T) {
          cost[vars[t].q] = c[t];
          cost[vars[t].theta] = 1.0;
        }

        cost[vars[t].I] = h[t - 1];
        cost[vars[t].B] = b[t - 1];

        m.changeColsCost(0, m.getLp().num_col_ - 1, cost.data());

        m.run();

        auto sol = m.getSolution();

        I_values[iter][t - 1][n] = sol.col_value[vars[t].I];
        B_values[iter][t - 1][n] = sol.col_value[vars[t].B];

        if (t < T)
          q_values[iter][t][n] = sol.col_value[vars[t].q];
      }
    }

    // ===== BACKWARD =====
    for (int t = T; t >= 1; t--) {
      for (int n = 0; n < forward_num; n++) {

        vector<double> slope_list;
        vector<double> intercept_list;

        for (int s = 0; s < sample_num; s++) {

          double demand = sample_details[t - 1][s];

          double rhs;
          if (t == 1)
            rhs = ini_I - demand + q_values[iter][t - 1][n];
          else
            rhs = I_values[iter][t - 2][n] - B_values[iter][t - 2][n] + q_values[iter][t - 1][n] -
                  demand;

          Highs &m = models[t];

          m.changeRowBounds(balance_row[t], rhs, rhs);
          m.run();

          auto sol = m.getSolution();
          double pi = sol.row_dual[balance_row[t]];

          slope_list.push_back(pi);
          intercept_list.push_back(-pi * demand);
        }

        slopes[iter][t - 1][n] = mean_val(slope_list);
        intercepts[iter][t - 1][n] = mean_val(intercept_list);

        // ===== 加 Benders cut =====
        if (t > 0 && t <= T - 1) {

          Highs &m_prev = models[t - 1];

          int ind[4] = {vars[t - 1].theta, vars[t - 1].q, vars[t - 1].I, vars[t - 1].B};

          double slope = slopes[iter][t - 1][n];

          double val[4] = {
              1.0,    // theta
              -slope, // q
              -slope, // I
              +slope  // B
          };

          m_prev.addRow(intercepts[iter][t - 1][n], kHighsInf, 4, ind, val);
        }
      }
    }

    cout << "iter " << iter << " obj = " << models[0].getObjectiveValue() << endl;
  }

  cout << "Final obj = " << models[0].getObjectiveValue() << endl;

  return 0;
}