//
// Created by Administrator on 2025/7/6.
//

#include "piecewise.h"
#include "../../utils/common.h"
#include "gurobi_c++.h"
#include <algorithm> // std::max_element
#include <boost/math/distributions/binomial.hpp> // for binomial distribution cdf and pdf, random library only for generating random numbers
#include <numeric> // for using accumulate

double PiecewiseWorkforce::loss_function(const int y, const int min_workers,
                                         const double turnover_rate) {
  const boost::math::binomial_distribution<double> dist(y, turnover_rate);
  const int i_min = std::max(y - min_workers, 0);
  double value = 0;
  for (int i = i_min; i <= y; i++) {
    value += pdf(dist, i) * (i + min_workers - y);
  }
  return value;
}

double PiecewiseWorkforce::Fy(const int y, const int min_workers, const double turnover_rate) {
  const boost::math::binomial_distribution<double> dist(y, turnover_rate);
  return cdf(dist, y - min_workers);
}

std::vector<std::vector<double>>
PiecewiseWorkforce::piecewise(const int segment_num, const int min_workers, const double p) {
  std::vector<double> slopes(segment_num + 1);
  std::vector<double> intercepts(segment_num + 1);
  std::vector<double> tangent_xcoord(segment_num + 1);
  std::vector<double> tangent_ycoord(segment_num + 1);
  std::vector<double> intercept_xcoord(segment_num + 1); // intercepts of the two adjoint lines
  std::vector<double> intercept_ycoord(segment_num + 1);
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
        if (Fy(j, min_workers, p) - Fy(a, min_workers, p) > 1.0 / segment_num) {
          tangent_xcoord[i] = j;
          const int b = static_cast<int>(tangent_xcoord[i]);
          tangent_ycoord[i] = loss_function(b, min_workers, p);
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
    const double y = loss_function(static_cast<int>(intercept_xcoord[i + 1]), min_workers, p);
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

double PiecewiseWorkforce::piece_approximate(const int segment_num) {
  try {
    // gurobi environments and model
    GRBEnv env;
    env.set(GRB_IntParam_OutputFlag, 0);
    auto model = GRBModel(env);

    // Create variables
    std::vector<GRBVar> y(T);
    std::vector<GRBVar> u(T);
    std::vector<GRBVar> x(T);
    std::vector<GRBVar> z(T);
    std::vector<std::vector<GRBVar>> P(T);

    for (int t = 0; t < T; t++) {
      P[t].resize(T);
      y[t] = model.addVar(0, GRB_INFINITY, unit_vari_cost, GRB_CONTINUOUS, "y");
      x[t] = model.addVar(0, GRB_INFINITY, salary, GRB_CONTINUOUS, "xt");
      u[t] = model.addVar(0, GRB_INFINITY, unit_penalty, GRB_CONTINUOUS, "u");
      z[t] = model.addVar(0, 1, fix_hire_cost, GRB_BINARY, "z");
      if (t > 0)
        x[t - 1] = model.addVar(0, GRB_INFINITY, -unit_vari_cost, GRB_CONTINUOUS, "xt-1");
    }

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
        model.addConstr(y[t] - initial_workers >= 0);
        model.addConstr(y[t] - initial_workers <= z[t] * M);
      }

      // sum_{j=1}^t P_{jt} == 1
      GRBLinExpr left = 0;
      for (int j = 0; j < t; j++) {
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
          p = p * (1 - turnover_rates[k]);
        auto result = piecewise(segment_num, min_workers[t], 1 - p);
        auto slopes = result[0];
        auto intercepts = result[1];
        auto gaps = result[6];
        double error = *std::ranges::max_element(gaps);

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
    double P_value = P[0][0].get(GRB_DoubleAttr_X);
    std::cout << "values of z are:" << std::endl;
    vectorToString(z_values);
    std::cout << "values of x are:" << std::endl;
    vectorToString(x_values);
    std::cout << "values of y are:" << std::endl;
    vectorToString(y_values);
    std::cout << "values of u are:" << std::endl;
    vectorToString(u_values);
    return this_obj;

  } catch (GRBException &e) {
    std::cout << "Error code = " << e.getErrorCode() << std::endl;
    std::cout << e.getMessage() << std::endl;
  } catch (...) {
    std::cout << "Exception during optimization" << std::endl;
  }
  return 0;
}
