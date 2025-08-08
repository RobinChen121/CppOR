//
// Created by Administrator on 2025/7/6.
//

#include "piecewise.h"
#include "../../utils/common.h"
#include "gurobi_c++.h"
#include "util_binomial.h"
// #include <algorithm> // std::max_element
#include <boost/math/distributions/binomial.hpp> // for binomial distribution cdf and pdf, random library only for generating random numbers
#include <numeric> // for using accumulate

std::vector<std::vector<double>>
PiecewiseWorkforce::piecewise(const int segment_num, const int min_workers, const double p) {
  std::vector<double> slopes(segment_num + 1);
  std::vector<double> intercepts(segment_num + 1);
  std::vector<double> tangent_xcoord(segment_num + 1);
  std::vector<double> tangent_ycoord(segment_num + 1);
  std::vector<double> intercept_xcoord(segment_num + 2); // intercepts of the two adjoining lines
  std::vector<double> intercept_ycoord(segment_num + 2);
  std::vector<double> intercept_gap(segment_num + 2);
  std::vector<std::vector<double>> result(7);

  int end_x = min_workers * 50; // can affect results
  for (int k = min_workers + 1; k < end_x; k++) {
    if (Fy(k, min_workers, p) > 0.9999) {
      end_x = k;
      break;
    }
  }

  slopes[segment_num] = 0;
  tangent_xcoord[segment_num] = end_x;
  tangent_ycoord[segment_num] = 0;
  intercepts[segment_num] = 0; // intercept is actually the y-intercept

  for (int i = 0; i < segment_num; i++) {
    if (i == 0) {
      slopes[i] = p - 1;
      tangent_xcoord[0] = min_workers - 1;
      tangent_ycoord[0] = (min_workers - 1) * p + 1; // right
      intercepts[0] = min_workers;
    } else {
      const int a = static_cast<int>(tangent_xcoord[i - 1]);
      tangent_xcoord[i] = a;
      slopes[i] = slopes[i - 1];

      for (int j = a + 1; j <= end_x; j++) {
        // double test = 1.0 / segment_num;
        // double test1 = Fy(j, min_workers, p);
        // double test2 = Fy(a, min_workers, p);
        if (Fy(j, min_workers, p) - Fy(a, min_workers, p) > 1.0 / segment_num) {
          tangent_xcoord[i] = j;
          const int b = static_cast<int>(tangent_xcoord[i]);
          tangent_ycoord[i] = loss_function_expect(b, min_workers, p);
          slopes[i] = -(1 - p) * (1 - Fy(b, min_workers, p));
          intercepts[i] = -slopes[i] * tangent_xcoord[i] + tangent_ycoord[i];
          break;
        }
      }
    }
  }

  intercept_xcoord[0] = 0;
  intercept_ycoord[0] = min_workers * p;
  intercept_gap[0] = 0;
  intercept_xcoord[segment_num + 1] = end_x;
  intercept_ycoord[segment_num + 1] = 0;
  intercept_gap[segment_num + 1] = 0;
  for (int i = 0; i < segment_num; i++) {
    intercept_xcoord[i + 1] = tangent_ycoord[i + 1] - tangent_ycoord[i] +
                              slopes[i] * tangent_xcoord[i] - slopes[i + 1] * tangent_xcoord[i + 1];
    intercept_xcoord[i + 1] = slopes[i] == slopes[i + 1]
                                  ? tangent_xcoord[i]
                                  : intercept_xcoord[i + 1] / (slopes[i] - slopes[i + 1]);
    intercept_ycoord[i + 1] =
        slopes[i] * (intercept_xcoord[i + 1] - tangent_xcoord[i]) + tangent_ycoord[i];
    const double y =
        loss_function_expect(static_cast<int>(intercept_xcoord[i + 1]), min_workers, p);
    intercept_gap[i + 1] = y - intercept_ycoord[i + 1];
  }

  result[0] = slopes;
  result[1] = intercepts;
  result[2] = tangent_xcoord;
  result[3] = tangent_ycoord;
  result[4] = intercept_xcoord;
  result[5] = intercept_ycoord;
  result[6] = intercept_gap;
  return result;
}

double PiecewiseWorkforce::piece_approximate(const int segment_num) const {
  try {
    // gurobi environments and model
    auto env = GRBEnv(true); // create an empty environment
    env.set(GRB_IntParam_OutputFlag, 0);
    env.start(); // necessary
    auto model = GRBModel(env);

    // Create variables
    std::vector<GRBVar> y(T);
    std::vector<GRBVar> u(T);
    std::vector<GRBVar> x(T);
    std::vector<GRBVar> z(T);
    std::vector<std::vector<GRBVar>> P(T);

    std::string var_name;
    for (int t = 0; t < T; t++) {
      P[t].resize(T);
      var_name = "y_" + std::to_string(t);
      y[t] = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, var_name);
      var_name = "x_" + std::to_string(t);
      x[t] = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, var_name);
      var_name = "u_" + std::to_string(t);
      u[t] = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, var_name);
      var_name = "z_" + std::to_string(t);
      z[t] = model.addVar(0, 1, 0.0, GRB_BINARY, var_name);
      for (int j = 0; j <= t; j++) {
        var_name = "P_" + std::to_string(j) + "_" + std::to_string(t);
        P[j][t] = model.addVar(0.0, 1, 0.0, GRB_BINARY, var_name);
      }
    }

    // objective function, set objective
    GRBLinExpr obj = 0;
    for (int t = 0; t < T; t++) {
      // using += results in errors for C++ gurobi api
      obj = obj + fix_hire_cost * z[t];
      if (t == 0) {
        obj = obj + unit_vari_cost * (y[t] - initial_workers);
      } else {
        obj = obj + unit_vari_cost * (y[t] - x[t - 1]);
      }
      obj = obj + unit_penalty * u[t];
      obj = obj + salary * x[t];
    }
    model.setObjective(obj, GRB_MINIMIZE);

    // constraints
    // M can not be too large, or else a slight difference of P[j][t] affects results
    const int M = initial_workers + 50 * std::accumulate(min_workers.begin(), min_workers.end(), 0);
    for (int t = 0; t < T; t++) {
      // y_t - x_{t-1} >= 0
      // y_t - x_{t-1} <= z_t M
      if (t == 0) {
        model.addConstr(y[t] - initial_workers >= 0);
        model.addConstr(y[t] - initial_workers <= z[t] * M);
      } else {
        model.addConstr(y[t] - x[t - 1] >= 0);
        model.addConstr(y[t] - x[t - 1] <= z[t] * M);
      }

      // sum_{j=1}^t P_{jt} == 1
      GRBLinExpr left = 0;
      for (int j = 0; j <= t; j++) {
        left += P[j][t];
      }
      model.addConstr(left == 1);

      // P_{jt} >= z_j - \sum_{k=j+1}^t z[k]
      for (int j = 0; j <= t; j++) {
        GRBLinExpr right = 0;
        for (int k = j + 1; k <= t; k++)
          right += -z[k];
        right += z[j];
        model.addConstr(P[j][t] >= right);
      }

      // x_t >= y_j(1-p)^{t-j+1} - (1-P_{jt})M
      // x_t <= y_j(1-p)^{t-j+1} + (1-P_{jt})M
      // revise
      for (int j = 0; j <= t; j++) {
        double p = 1;
        for (int k = j; k <= t; k++)
          p = p * (1 - turnover_rates[k]);
        GRBLinExpr right2;
        right2 = y[j] * p - (1 - P[j][t]) * M;
        model.addConstr(x[t] >= right2);
        GRBLinExpr right3;
        right3 = y[j] * p + (1 - P[j][t]) * M;
        model.addConstr(x[t] <= right3);
      }

      // piecewise constraints
      // u_t >= \alpha y_j + \beta - (1 - P_{jt})M
      // something wrong in the piecewise for u[t]
      for (int j = 0; j <= t; j++) {
        double p = 1;
        for (int k = j; k <= t; k++)
          p = p * (1 - turnover_rates[k]); // p *
        auto result = piecewise(segment_num, min_workers[t], 1 - p);
        const auto &slopes = result[0];
        const auto &intercepts = result[1];
        // auto gaps = result[6];
        // double error = *std::ranges::max_element(gaps);

        for (int m = 0; m < segment_num; m++) {
          // lower bound
          model.addConstr(u[t] >= slopes[m] * y[j] + intercepts[m] + M * (P[j][t] - 1));

          // // upper bound
          // model.addConstr(u[t] >= slopes[m] * y[j] + intercepts[m] + M * (P[j][t] - 1) + error);
        }
      }
    }

    // Optimize model
    model.optimize();
    // model.write("piecewise.lp");

    // output results
    double this_obj = model.get(GRB_DoubleAttr_ObjVal);
    std::cout << "objective value of the mip model is " << this_obj << std::endl;
    std::vector<double> x_values(T);
    std::vector<double> y_values(T);
    std::vector<double> u_values(T);
    std::vector<int> z_values(T);
    for (int t = 0; t < T; t++) {
      y_values[t] = y[t].get(GRB_DoubleAttr_X);
      x_values[t] = x[t].get(GRB_DoubleAttr_X);
      u_values[t] = u[t].get(GRB_DoubleAttr_X);
      z_values[t] = z[t].get(GRB_DoubleAttr_X);
    }
    // double P_value = P[0][0].get(GRB_DoubleAttr_X);
    std::cout << "values of z are: " << vectorToString(z_values) << std::endl;
    std::cout << "values of x are: " << vectorToString(x_values) << std::endl;
    std::cout << "values of y are: " << vectorToString(y_values) << std::endl;
    std::cout << "values of u are: " << vectorToString(u_values) << std::endl;
    return this_obj;

  } catch (GRBException &e) {
    std::cout << "Error code = " << e.getErrorCode() << std::endl;
    std::cout << e.getMessage() << std::endl;
  } catch (...) {
    std::cout << "Exception during optimization" << std::endl;
  }
  return 0;
}

std::vector<std::array<int, 2>> PiecewiseWorkforce::get_sS(int segment_num) const {
  std::vector<std::array<int, 2>> sS(T);
  for (int tt = 0; tt < T; tt++) {
    try {
      // gurobi environments and model
      auto env = GRBEnv(true); // create an empty environment
      env.set(GRB_IntParam_OutputFlag, 0);
      env.start(); // necessary
      auto model = GRBModel(env);

      std::vector<GRBVar> y(T - tt);
      std::vector<GRBVar> u(T - tt);
      std::vector<GRBVar> x(T - tt);
      std::vector<GRBVar> z(T - tt);
      std::vector<std::vector<GRBVar>> P(T - tt);

      std::string var_name;
      for (int t = 0; t < T - tt; t++) {
        P[t].resize(T - tt);
        var_name = "y_" + std::to_string(t);
        y[t] = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, var_name);
        var_name = "x_" + std::to_string(t);
        x[t] = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, var_name);
        var_name = "u_" + std::to_string(t);
        u[t] = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, var_name);
        var_name = "z_" + std::to_string(t);
        z[t] = model.addVar(0, 1, 0.0, GRB_BINARY, var_name);
        for (int j = 0; j <= t; j++) {
          var_name = "P_" + std::to_string(j) + "_" + std::to_string(t);
          P[j][t] = model.addVar(0, 1, 0.0, GRB_BINARY, var_name);
        }
      }
      GRBVar S = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, "S");

      // objective function, set objective
      GRBLinExpr obj;
      for (int t = 0; t < T - tt; t++) {
        // using += results in errors for C++ gurobi api
        obj = obj + fix_hire_cost * z[t];
        if (t == 0) {
          obj = obj + unit_vari_cost * (y[t] - S);
        } else {
          obj = obj + unit_vari_cost * (y[t] - x[t - 1]);
        }
        obj = obj + unit_penalty * u[t] + salary * x[t];
      }
      model.setObjective(obj, GRB_MINIMIZE);

      // constraints
      // z[0] == 0
      model.addConstr(z[0] == 0);

      // M can not be too large, or a slight difference of P[j][t] affects results
      const int M =
          initial_workers + 50 * std::accumulate(min_workers.begin(), min_workers.end(), 0);
      for (int t = 0; t < T - tt; t++) {
        // y_t - x_{t-1} >= 0
        // y_t - x_{t-1} <= z_t M
        if (t == 0) {
          model.addConstr(y[t] - S >= 0);
          model.addConstr(y[t] - S <= z[t] * M);
        } else {
          model.addConstr(y[t] - x[t - 1] >= 0);
          model.addConstr(y[t] - x[t - 1] <= z[t] * M);
        }

        // sum_{j=1}^t P_{jt} == 1
        GRBLinExpr left = 0;
        for (int j = 0; j <= t; j++) {
          left += P[j][t];
        }
        model.addConstr(left == 1);

        // P_{jt} >= z_j - \sum_{k=j+1}^t z[k]
        for (int j = 0; j <= t; j++) {
          GRBLinExpr right = 0;
          for (int k = j + 1; k <= t; k++)
            right += -z[k];
          right += z[j];
          model.addConstr(P[j][t] >= right);
        }

        // x_t >= y_j(1-p)^{t-j+1} - (1-P_{jt})M
        // x_t <= y_j(1-p)^{t-j+1} + (1-P_{jt})M
        // revise
        for (int j = 0; j <= t; j++) {
          double p = 1;
          for (int k = j; k <= t; k++)
            p = p * (1 - turnover_rates[k]);
          GRBLinExpr right2;
          right2 = y[j] * p - (1 - P[j][t]) * M;
          model.addConstr(x[t] >= right2);
          GRBLinExpr right3;
          right3 = y[j] * p + (1 - P[j][t]) * M;
          model.addConstr(x[t] <= right3);
        }

        // piecewise constraints
        // u_t >= \alpha y_j + \beta - (1 - P_{jt})M
        // something wrong in the piecewise for u[t]
        for (int j = 0; j <= t; j++) {
          double p = 1;
          for (int k = j; k <= t; k++)
            p = p * (1 - turnover_rates[k + tt]);
          auto result = piecewise(segment_num, min_workers[t + tt], 1 - p);
          const auto &slopes = result[0];
          const auto &intercepts = result[1];
          // auto gaps = result[6];
          // double error = *std::ranges::max_element(gaps);

          for (int m = 0; m < segment_num; m++) {
            // lower bound
            model.addConstr(u[t] >= slopes[m] * y[j] + intercepts[m] + M * (P[j][t] - 1));
            // // upper bound
            // model.addConstr(u[t] >= slopes[m] * y[j] + intercepts[m] + M * (P[j][t] - 1) +
            // error);
          }
        }
      }

      // Optimize model
      model.optimize();
      // model.write("piecewise.lp");
      // int status = model.get(GRB_IntAttr_Status);

      // output results
      int S_value = static_cast<int>(S.get(GRB_DoubleAttr_X));
      double GS = model.get(GRB_DoubleAttr_ObjVal) + unit_vari_cost * S.get(GRB_DoubleAttr_X);
      sS[tt][1] = static_cast<int>(S_value);

      // find s
      int s = find_s(segment_num, S_value, GS, tt);
      sS[tt][0] = static_cast<int>(s);

    } catch (GRBException &e) {
      std::cout << "Error code = " << e.getErrorCode() << std::endl;
      std::cout << e.getMessage() << std::endl;
    } catch (...) {
      std::cout << "Exception during optimization" << std::endl;
    }
  }
  return sS;
}

int PiecewiseWorkforce::find_s(int segment_num, int S_value, double GS, int tt) const {
  double low = 0;
  double high = S_value;
  double stepSize = 1;
  double mid;
  while (low < high) {
    mid = std::round((high + low) / 2.0);
    try {
      // gurobi environments and model
      auto env = GRBEnv(true); // create an empty environment
      env.set(GRB_IntParam_OutputFlag, 0);
      env.start(); // necessary
      auto model = GRBModel(env);

      std::vector<GRBVar> y(T - tt);
      std::vector<GRBVar> u(T - tt);
      std::vector<GRBVar> x(T - tt);
      std::vector<GRBVar> z(T - tt);
      std::vector<std::vector<GRBVar>> P(T - tt);

      std::string var_name;
      for (int t = 0; t < T - tt; t++) {
        P[t].resize(T - tt);
        var_name = "y_" + std::to_string(t);
        y[t] = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, var_name);
        var_name = "x_" + std::to_string(t);
        x[t] = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, var_name);
        var_name = "u_" + std::to_string(t);
        u[t] = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, var_name);
        var_name = "z_" + std::to_string(t);
        z[t] = model.addVar(0, 1, 0.0, GRB_BINARY, var_name);
        for (int j = 0; j <= t; j++) {
          var_name = "P_" + std::to_string(j) + "_" + std::to_string(t);
          P[j][t] = model.addVar(0, 1, 0.0, GRB_BINARY, var_name);
        }
      }
      GRBVar S = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, "S");

      // objective function, set objective
      GRBLinExpr obj;
      for (int t = 0; t < T - tt; t++) {
        // using += results in errors for C++ gurobi api
        obj = obj + fix_hire_cost * z[t];
        if (t == 0) {
          obj = obj + unit_vari_cost * (y[t] - S);
        } else {
          obj = obj + unit_vari_cost * (y[t] - x[t - 1]);
        }
        obj = obj + unit_penalty * u[t] + salary * x[t];
      }
      model.setObjective(obj, GRB_MINIMIZE);

      // constraints
      // z[0] == 0
      model.addConstr(z[0] == 0);
      // S == mid
      model.addConstr(S == mid);

      // M can not be too large, or a slight difference of P[j][t] affects results
      const int M =
          initial_workers + 50 * std::accumulate(min_workers.begin(), min_workers.end(), 0);
      for (int t = 0; t < T - tt; t++) {
        // y_t - x_{t-1} >= 0
        // y_t - x_{t-1} <= z_t M
        if (t == 0) {
          model.addConstr(y[t] - S >= 0);
          model.addConstr(y[t] - S <= z[t] * M);
        } else {
          model.addConstr(y[t] - x[t - 1] >= 0);
          model.addConstr(y[t] - x[t - 1] <= z[t] * M);
        }

        // sum_{j=1}^t P_{jt} == 1
        GRBLinExpr left = 0;
        for (int j = 0; j <= t; j++) {
          left += P[j][t];
        }
        model.addConstr(left == 1);

        // P_{jt} >= z_j - \sum_{k=j+1}^t z[k]
        for (int j = 0; j <= t; j++) {
          GRBLinExpr right = 0;
          for (int k = j + 1; k <= t; k++)
            right += -z[k];
          right += z[j];
          model.addConstr(P[j][t] >= right);
        }

        // x_t >= y_j(1-p)^{t-j+1} - (1-P_{jt})M
        // x_t <= y_j(1-p)^{t-j+1} + (1-P_{jt})M
        // revise
        for (int j = 0; j <= t; j++) {
          double p = 1;
          for (int k = j; k <= t; k++)
            p = p * (1 - turnover_rates[k]);
          GRBLinExpr right2;
          right2 = y[j] * p - (1 - P[j][t]) * M;
          model.addConstr(x[t] >= right2);
          GRBLinExpr right3;
          right3 = y[j] * p + (1 - P[j][t]) * M;
          model.addConstr(x[t] <= right3);
        }

        // piecewise constraints
        // u_t >= \alpha y_j + \beta - (1 - P_{jt})M
        // something wrong in the piecewise for u[t]
        for (int j = 0; j <= t; j++) {
          double p = 1;
          for (int k = j; k <= t; k++)
            p = p * (1 - turnover_rates[k + tt]);
          auto result = piecewise(segment_num, min_workers[t + tt], 1 - p);
          const auto &slopes = result[0];
          const auto &intercepts = result[1];
          // auto gaps = result[6];
          // double error = *std::ranges::max_element(gaps);

          for (int m = 0; m < segment_num; m++) {
            // lower bound
            model.addConstr(u[t] >= slopes[m] * y[j] + intercepts[m] + M * (P[j][t] - 1));

            // // upper bound
            // model.addConstr(u[t] >= slopes[m] * y[j] + intercepts[m] + M * (P[j][t] - 1) +
            // error);
          }
        }
      }

      // Optimize model
      model.optimize();
      // model.write("piecewise.lp");
      // model.write("piecewise.sol");
      // int status = model.get(GRB_IntAttr_Status);

      double G_mid =
              model.get(GRB_DoubleAttr_ObjVal) + unit_vari_cost * S.get(GRB_DoubleAttr_X);
      if (G_mid < GS + fix_hire_cost)
        high = mid - stepSize;
      else if (G_mid > GS + fix_hire_cost)
        low = mid + stepSize;
      else
        low = high;
    } catch (GRBException &e) {
      std::cout << "Error code = " << e.getErrorCode() << std::endl;
      std::cout << e.getMessage() << std::endl;
    } catch (...) {
      std::cout << "Exception during optimization" << std::endl;
    }
  }
  int s = static_cast<int>(mid - 1);
  return s;
}
