//
// Created by Administrator on 2025/7/6.
// Piecewise slopes and intercepts can be precomputed.
// The function 'preparePiecewiseCache' guarantees that they are computed only once for each
// segment_num.
//

#include "piecewise.h"
#include "../../utils/common.h"
#include "gurobi_c++.h"
#include "util_binomial.h"
// #include <algorithm> // std::max_element
#include "workforce_plan_new.h"

#include <numeric> // for using accumulate

void PiecewiseWorkforce::preparePiecewiseCache() const { preparePiecewiseCache(segment_num_); }
void PiecewiseWorkforce::preparePiecewiseCache(const int segment_num) const {
  if (!piecewise_cache_.empty()) {
    return;
  }

  piecewise_cache_.clear();
  // offset is the starting index for computing sS
  for (size_t t = 0; t < T; ++t) {
    for (int j = 0; j <= static_cast<int>(t); j++) {
      double survival = 1.0;
      for (int k = j; k <= t; ++k) {
        survival *= 1.0 - turnover_rates[k];
      }
      const auto key = PiecewiseKey{segment_num, static_cast<int>(t), j};
      piecewise_cache_[key] = piecewise(segment_num, min_workers[t], 1.0 - survival);

      // compute segment 2
      if (std::abs(segment_num - 2) > 1e-3) {
        // For segment_num = 2, we need to compute the piecewise function as well
        const auto key2 = PiecewiseKey{2, static_cast<int>(t), j};
        piecewise_cache_[key2] = piecewise(2, min_workers[t], 1.0 - survival);
      }
    }
  }
}

const PiecewiseWorkforce::PiecewiseResult &
PiecewiseWorkforce::getPiecewiseResult(const int segment_num, const int t, const int j) const {
  preparePiecewiseCache(segment_num);
  const auto key = PiecewiseKey{segment_num, t, j};
  return piecewise_cache_.at(key);
}

PiecewiseWorkforce::PiecewiseResult
PiecewiseWorkforce::piecewise(const int segment_num, const int min_worker, const double p) {

  std::vector<double> slopes(segment_num + 1);
  std::vector<double> intercepts(segment_num + 1);
  std::vector<double> tangent_xcoord(segment_num + 1);
  std::vector<double> tangent_ycoord(segment_num + 1);
  std::vector<double> intercept_xcoord(segment_num + 2); // intercepts of the two adjoining lines
  std::vector<double> intercept_ycoord(segment_num + 2);
  std::vector<double> intercept_gap(segment_num + 2);
  std::vector<std::vector<double>> result(7);

  int end_x = min_worker * 50; // can affect results
  for (int k = min_worker + 1; k < end_x; k++) {
    if (Fy_y_minus_w(k, min_worker, p) > 0.9999) {
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
      tangent_xcoord[0] = min_worker - 1;
      tangent_ycoord[0] = (min_worker - 1) * p + 1; // right
      intercepts[0] = min_worker;
    } else {
      const auto a = std::round(tangent_xcoord[i - 1]);
      tangent_xcoord[i] = static_cast<double>(a);
      slopes[i] = slopes[i - 1];

      for (int j = static_cast<int>(a) + 1; j <= end_x; j++) {
        // double test = 1.0 / segment_num;
        // double test1 = Fy(j, min_worker, p);
        // double test2 = Fy(a, min_worker, p);
        if (Fy_y_minus_w(j, min_worker, p) - Fy_y_minus_w(static_cast<int>(a), min_worker, p) >
            1.0 / segment_num) {
          tangent_xcoord[i] = j;
          const int b = static_cast<int>(std::round(tangent_xcoord[i]));
          tangent_ycoord[i] = lossFunctionExpect(b, min_worker, p);
          slopes[i] = -(1 - p) * (1 - Fy_y_minus_w(b, min_worker, p));
          intercepts[i] = -slopes[i] * tangent_xcoord[i] + tangent_ycoord[i];
          break;
        }
      }
    }
  }

  intercept_xcoord[0] = 0;
  intercept_ycoord[0] = min_worker * p;
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
    const double y = lossFunctionExpect(std::round(intercept_xcoord[i + 1]), min_worker, p);
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

std::pair<double, double> PiecewiseWorkforce::pieceApproximateCallback() const {
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
    std::vector P(T, std::vector<GRBVar>(T));

    std::string var_name;
    for (int t = 0; t < T; t++) {
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

    // call back 需要定义一个类或结构体继承 GRBCallback，并重写 callback() 方法
    struct LazyCallback final : GRBCallback {
      const PiecewiseWorkforce *self;
      const int M;
      std::vector<GRBVar> *y;
      std::vector<GRBVar> *u;
      std::vector<std::vector<GRBVar>> *P;

      LazyCallback(const PiecewiseWorkforce *self, const int M, std::vector<GRBVar> *y,
                   std::vector<GRBVar> *u, std::vector<std::vector<GRBVar>> *P)
          : self(self), M(M), y(y), u(u), P(P) {}

      void callback() override {
        if (where != GRB_CB_MIPSOL) { // 若没有找到整数可行解，则不需要添加 lazy constraints
          return;
        }

        for (int t = 0; t < static_cast<int>(self->T); ++t) {
          for (int j = 0; j <= t; ++j) {
            const double y_val = getSolution((*y)[j]);
            const int this_P = std::round(getSolution((*P)[j][t]));
            if (this_P < 0.5) {
              continue; // if P[j][t] = 0, then the constraint is not active, no need to add lazy
              // constraint
            }
            for (int k = j; k <= t; ++k) {
              const double u_val = getSolution((*u)[k]);
              const int min_worker = self->min_workers[k];
              double p = 1.0; // p is q_{jt} in the paper
              for (int m = j; m <= k; ++m) {
                p *= 1.0 - self->turnover_rates[m];
              }
              const double loss_val = lossFunctionExpect(y_val, min_worker, 1.0 - p);
              if (loss_val - u_val > 1e-3) { // EPS
                const double slope = -p * (1 - Fy_y_minus_w(static_cast<int>(std::round(y_val)),
                                                            min_worker, 1.0 - p));
                const double intercept = loss_val - slope * y_val;
                // User cuts help tighten the relaxation of a MIP by removing fractional solutions.
                // They are not required for the model,but they potentially help solve a MIP
                // faster. Lazy constraints are required for the model, i.e., the model would be
                // incorrect without these constraints.For some models, it is helpful to designate
                // some constraints as lazy when it is computationally faster to include them only
                // when they are violated. Generally, they are used for models that contain a
                // relatively large number of constraints, most of which are trivially satisfied.
                addLazy((*u)[k] >= slope * (*y)[j] + intercept + M * ((*P)[j][t] - 1));
                // std::cout << "lazy added " << j << " " << k << std::endl;
              }
            }
          }
        }
      }
    };

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
    const int M = initial_workers + T * std::accumulate(min_workers.begin(), min_workers.end(), 0);
    LazyCallback cb(this, M, &y, &u, &P);
    model.set(GRB_IntParam_LazyConstraints, 1);
    model.setCallback(&cb);
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

        // initial piecewise lower bound
        model.addConstr(u[t] >= -p * y[j] + min_workers[t] + M * (P[j][t] - 1));
        // initial 2nd cut
        const auto &result = getPiecewiseResult(2, t, j);
        const auto &slopes = result[0];
        const auto &intercepts = result[1];
        model.addConstr(u[t] >= slopes[1] * y[j] + intercepts[1] + M * (P[j][t] - 1));
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
      z_values[t] = std::round(z[t].get(GRB_DoubleAttr_X));
    }
    // double P_value = P[0][0].get(GRB_DoubleAttr_X);
    std::cout << "values of z are: " << vectorToString(z_values) << std::endl;
    std::cout << "values of x are: " << vectorToString(x_values) << std::endl;
    std::cout << "values of y are: " << vectorToString(y_values) << std::endl;
    std::cout << "values of u are: " << vectorToString(u_values) << std::endl;
    return {this_obj, computeLineGap(z_values, y_values, u_values)};

  } catch (GRBException &e) {
    std::cout << "Error code = " << e.getErrorCode() << std::endl;
    std::cout << e.getMessage() << std::endl;
  } catch (...) {
    std::cout << "Exception during optimization" << std::endl;
  }
  return {0.0, 0.0};
}

std::pair<double, double> PiecewiseWorkforce::pieceApproximate(const int segment_num) const {
  try {
    // preparePiecewiseCache(segment_num);
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
    std::vector P(T, std::vector<GRBVar>(T));

    std::string var_name;
    for (int t = 0; t < T; t++) {
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
    const int M = initial_workers + T * std::accumulate(min_workers.begin(), min_workers.end(), 0);
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
      for (int j = 0; j <= t; j++) {
        const auto &result = getPiecewiseResult(segment_num, t, j);
        const auto &slopes = result[0];
        const auto &intercepts = result[1];

        for (int m = 0; m < segment_num; m++) {
          // lower bound
          model.addConstr(u[t] >= slopes[m] * y[j] + intercepts[m] + M * (P[j][t] - 1));
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
      z_values[t] = std::round(z[t].get(GRB_DoubleAttr_X));
    }
    // double P_value = P[0][0].get(GRB_DoubleAttr_X);
    std::cout << "values of z are: " << vectorToString(z_values) << std::endl;
    std::cout << "values of x are: " << vectorToString(x_values) << std::endl;
    std::cout << "values of y are: " << vectorToString(y_values) << std::endl;
    std::cout << "values of u are: " << vectorToString(u_values) << std::endl;
    return {this_obj, computeLineGap(z_values, y_values, u_values)};

  } catch (GRBException &e) {
    std::cout << "Error code = " << e.getErrorCode() << std::endl;
    std::cout << e.getMessage() << std::endl;
  } catch (...) {
    std::cout << "Exception during optimization" << std::endl;
  }
  return {0.0, 0.0};
}

double PiecewiseWorkforce::computeLineGap(const std::vector<int> &z, const std::vector<double> &y,
                                          const std::vector<double> &u) const {
  double gap = 0.0;
  for (int t = 0; t < T; t++) {
    if (std::abs(z[t] - 1) < 1e-6) {
      const double real_u = lossFunctionExpect(y[t], min_workers[t], turnover_rates[t]);
      gap += (real_u - u[t]) * unit_penalty;
    } else {
      int last_z = 0;
      double pc = 1 - turnover_rates[t];
      for (int j = t - 1; j >= 0; j--) {
        pc *= 1 - turnover_rates[j];
        if (std::abs(z[j] - 1) < 1e-6) {
          last_z = j;
          break;
        }
      }
      const double real_u = lossFunctionExpect(y[last_z], min_workers[t], 1 - pc);
      gap += (real_u - u[t]) * unit_penalty;
    }
  }
  return gap;
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

      // M can not be too large, or else a slight difference of P[j][t] affects results
      const int M =
          initial_workers + T * std::accumulate(min_workers.begin(), min_workers.end(), 0);
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
            p = p * (1 - turnover_rates[k + tt]);
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
          const auto &result = getPiecewiseResult(segment_num, tt + t, tt + j);
          const auto &slopes = result[0];
          const auto &intercepts = result[1];

          for (int m = 0; m < segment_num; m++) {
            // lower bound
            model.addConstr(u[t] >= slopes[m] * y[j] + intercepts[m] + M * (P[j][t] - 1));
          }
        }
      }

      // Optimize model
      model.optimize();
      // model.write("piecewise.lp");
      // int status = model.get(GRB_IntAttr_Status);

      // output results
      double S_value = S.get(GRB_DoubleAttr_X);
      double GS = model.get(GRB_DoubleAttr_ObjVal) + unit_vari_cost * S.get(GRB_DoubleAttr_X);
      sS[tt][1] = std::round(
          S_value); // 不能用 static_cast<int>(S_value) 直接转换为int，因为它会丢弃小数部分

      // find s
      int s = find_s(segment_num, sS[tt][1], GS, tt);
      sS[tt][0] = s;

    } catch (GRBException &e) {
      std::cout << "Error code = " << e.getErrorCode() << std::endl;
      std::cout << e.getMessage() << std::endl;
    } catch (...) {
      std::cout << "Exception during optimization" << std::endl;
    }
  }
  return sS;
}

std::vector<std::array<int, 2>> PiecewiseWorkforce::get_sS_callback() const {
  std::vector<std::array<int, 2>> sS(T);
  for (int tt = 0; tt < T; tt++) {
    try {
      auto env = GRBEnv(true);
      env.set(GRB_IntParam_OutputFlag, 0);
      env.start();
      auto model = GRBModel(env);

      const int horizon = static_cast<int>(T) - tt;
      std::vector<GRBVar> y(horizon);
      std::vector<GRBVar> u(horizon);
      std::vector<GRBVar> x(horizon);
      std::vector<GRBVar> z(horizon);
      std::vector P(horizon, std::vector<GRBVar>(horizon));

      std::string var_name;
      for (int t = 0; t < horizon; t++) {
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
      GRBVar S = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, "S");

      GRBLinExpr obj = 0;
      for (int t = 0; t < horizon; t++) {
        obj = obj + fix_hire_cost * z[t];
        if (t == 0) {
          obj = obj + unit_vari_cost * (y[t] - S);
        } else {
          obj = obj + unit_vari_cost * (y[t] - x[t - 1]);
        }
        obj = obj + unit_penalty * u[t] + salary * x[t];
      }
      model.setObjective(obj, GRB_MINIMIZE);

      model.addConstr(z[0] == 0);
      const int M = initial_workers + static_cast<int>(T) * std::accumulate(min_workers.begin(),
                                                                            min_workers.end(), 0);

      struct LazyCallbackSS final : GRBCallback {
        const PiecewiseWorkforce *self;
        const int M;
        const int tt;
        std::vector<GRBVar> *y;
        std::vector<GRBVar> *u;
        std::vector<std::vector<GRBVar>> *P;

        LazyCallbackSS(const PiecewiseWorkforce *self, const int M, const int tt,
                       std::vector<GRBVar> *y, std::vector<GRBVar> *u,
                       std::vector<std::vector<GRBVar>> *P)
            : self(self), M(M), tt(tt), y(y), u(u), P(P) {}

        void callback() override {
          if (where != GRB_CB_MIPSOL)
            return;

          const int horizon = static_cast<int>(self->T) - tt;
          for (int t = 0; t < horizon; ++t) {
            for (int j = 0; j <= t; ++j) {
              const double y_val = getSolution((*y)[j]);
              const int this_P = std::round(getSolution((*P)[j][t]));
              if (this_P < 0.5)
                continue;
              for (int k = j; k <= t; ++k) {
                const double u_val = getSolution((*u)[k]);
                const int min_worker = self->min_workers[tt + k];
                double p = 1.0;
                for (int m = j; m <= k; ++m) {
                  p *= 1.0 - self->turnover_rates[tt + m];
                }
                const double loss_val = lossFunctionExpect(y_val, min_worker, 1.0 - p);
                if (loss_val - u_val > 1e-6) {
                  const double slope = -p * (1 - Fy_y_minus_w(static_cast<int>(std::round(y_val)),
                                                              min_worker, 1.0 - p));
                  const double intercept = loss_val - slope * y_val;
                  addLazy((*u)[k] >= slope * (*y)[j] + intercept + M * ((*P)[j][t] - 1));
                }
              }
            }
          }
        }
      };

      LazyCallbackSS cb(this, M, tt, &y, &u, &P);
      model.set(GRB_IntParam_LazyConstraints, 1);
      model.setCallback(&cb);

      for (int t = 0; t < horizon; t++) {
        if (t == 0) {
          model.addConstr(y[t] - S >= 0);
          model.addConstr(y[t] - S <= z[t] * M);
        } else {
          model.addConstr(y[t] - x[t - 1] >= 0);
          model.addConstr(y[t] - x[t - 1] <= z[t] * M);
        }

        GRBLinExpr left = 0;
        for (int j = 0; j <= t; j++) {
          left += P[j][t];
        }
        model.addConstr(left == 1);

        for (int j = 0; j <= t; j++) {
          GRBLinExpr right = 0;
          for (int k = j + 1; k <= t; k++)
            right += -z[k];
          right += z[j];
          model.addConstr(P[j][t] >= right);
        }

        for (int j = 0; j <= t; j++) {
          double p = 1;
          for (int k = j; k <= t; k++)
            p = p * (1 - turnover_rates[k + tt]);
          GRBLinExpr right2;
          right2 = y[j] * p - (1 - P[j][t]) * M;
          model.addConstr(x[t] >= right2);
          GRBLinExpr right3;
          right3 = y[j] * p + (1 - P[j][t]) * M;
          model.addConstr(x[t] <= right3);

          // initial piecewise lower bound
          model.addConstr(u[t] >= -p * y[j] + min_workers[tt + t] + M * (P[j][t] - 1));
        }
      }

      model.optimize();
      double S_value = S.get(GRB_DoubleAttr_X);
      double GS = model.get(GRB_DoubleAttr_ObjVal) + unit_vari_cost * S_value;
      sS[tt][1] = std::round(S_value);
      sS[tt][0] = find_s_callback(sS[tt][1], GS, tt);
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
      const int M = static_cast<int>(
          initial_workers + T * std::accumulate(min_workers.begin(), min_workers.end(), 0));
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
            p = p * (1 - turnover_rates[k + tt]);
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
          const auto &result = getPiecewiseResult(segment_num, tt + t, tt + j);
          const auto &slopes = result[0];
          const auto &intercepts = result[1];

          for (int m = 0; m < segment_num; m++) {
            // lower bound
            model.addConstr(u[t] >= slopes[m] * y[j] + intercepts[m] + M * (P[j][t] - 1));
          }
        }
      }

      // Optimize model
      model.optimize();
      // model.write("piecewise.lp");
      // model.write("piecewise.sol");
      // int status = model.get(GRB_IntAttr_Status);

      double G_mid = model.get(GRB_DoubleAttr_ObjVal) + unit_vari_cost * S.get(GRB_DoubleAttr_X);
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
  int s = std::round(mid - 1);
  return s;
}

int PiecewiseWorkforce::find_s_callback(int S_value, double GS, int tt) const {
  double low = 0;
  double high = S_value;
  const double stepSize = 1;
  double mid = 0;
  while (low < high) {
    mid = std::round((high + low) / 2.0);
    try {
      auto env = GRBEnv(true);
      env.set(GRB_IntParam_OutputFlag, 0);
      env.start();
      auto model = GRBModel(env);

      const int horizon = static_cast<int>(T) - tt;
      std::vector<GRBVar> y(horizon);
      std::vector<GRBVar> u(horizon);
      std::vector<GRBVar> x(horizon);
      std::vector<GRBVar> z(horizon);
      std::vector P(horizon, std::vector<GRBVar>(horizon));

      std::string var_name;
      for (int t = 0; t < horizon; t++) {
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
      GRBVar S = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, "S");

      GRBLinExpr obj = 0;
      for (int t = 0; t < horizon; t++) {
        obj = obj + fix_hire_cost * z[t];
        if (t == 0) {
          obj = obj + unit_vari_cost * (y[t] - S);
        } else {
          obj = obj + unit_vari_cost * (y[t] - x[t - 1]);
        }
        obj = obj + unit_penalty * u[t] + salary * x[t];
      }
      model.setObjective(obj, GRB_MINIMIZE);

      model.addConstr(z[0] == 0);
      model.addConstr(S == mid);
      const int M = initial_workers + static_cast<int>(T) * std::accumulate(min_workers.begin(),
                                                                            min_workers.end(), 0);

      struct LazyCallbackFindS final : GRBCallback {
        const PiecewiseWorkforce *self;
        const int M;
        const int tt;
        std::vector<GRBVar> *y;
        std::vector<GRBVar> *u;
        std::vector<std::vector<GRBVar>> *P;

        LazyCallbackFindS(const PiecewiseWorkforce *self, const int M, const int tt,
                          std::vector<GRBVar> *y, std::vector<GRBVar> *u,
                          std::vector<std::vector<GRBVar>> *P)
            : self(self), M(M), tt(tt), y(y), u(u), P(P) {}

        void callback() override {
          if (where != GRB_CB_MIPSOL)
            return;

          const int horizon = static_cast<int>(self->T) - tt;
          for (int t = 0; t < horizon; ++t) {
            for (int j = 0; j <= t; ++j) {
              const double y_val = getSolution((*y)[j]);
              const int this_P = std::round(getSolution((*P)[j][t]));
              if (this_P < 0.5)
                continue;
              for (int k = j; k <= t; ++k) {
                const double u_val = getSolution((*u)[k]);
                const int min_worker = self->min_workers[tt + k];
                double p = 1.0;
                for (int m = j; m <= k; ++m) {
                  p *= 1.0 - self->turnover_rates[tt + m];
                }
                const double loss_val = lossFunctionExpect(y_val, min_worker, 1.0 - p);
                if (loss_val - u_val > 1e-6) {
                  const double slope = -p * (1 - Fy_y_minus_w(static_cast<int>(std::round(y_val)),
                                                              min_worker, 1.0 - p));
                  const double intercept = loss_val - slope * y_val;
                  addLazy((*u)[k] >= slope * (*y)[j] + intercept + M * ((*P)[j][t] - 1));
                }
              }
            }
          }
        }
      };

      LazyCallbackFindS cb(this, M, tt, &y, &u, &P);
      model.set(GRB_IntParam_LazyConstraints, 1);
      model.setCallback(&cb);

      for (int t = 0; t < horizon; t++) {
        if (t == 0) {
          model.addConstr(y[t] - S >= 0);
          model.addConstr(y[t] - S <= z[t] * M);
        } else {
          model.addConstr(y[t] - x[t - 1] >= 0);
          model.addConstr(y[t] - x[t - 1] <= z[t] * M);
        }

        GRBLinExpr left = 0;
        for (int j = 0; j <= t; j++) {
          left += P[j][t];
        }
        model.addConstr(left == 1);

        for (int j = 0; j <= t; j++) {
          GRBLinExpr right = 0;
          for (int k = j + 1; k <= t; k++)
            right += -z[k];
          right += z[j];
          model.addConstr(P[j][t] >= right);
        }

        for (int j = 0; j <= t; j++) {
          double p = 1;
          for (int k = j; k <= t; k++)
            p = p * (1 - turnover_rates[k + tt]);
          GRBLinExpr right2;
          right2 = y[j] * p - (1 - P[j][t]) * M;
          model.addConstr(x[t] >= right2);
          GRBLinExpr right3;
          right3 = y[j] * p + (1 - P[j][t]) * M;
          model.addConstr(x[t] <= right3);
          // initial piecewise lower bound
          model.addConstr(u[t] >= -p * y[j] + min_workers[tt + t] + M * (P[j][t] - 1));
        }
      }

      model.optimize();
      const double G_mid =
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
  return std::round(mid - 1);
}