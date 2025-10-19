/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 19/10/2025, 12:34
 * Description:
 *
 */

#ifndef ACTIVATION_H
#define ACTIVATION_H

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

struct ReLU {
  static double activate(const double x) { return std::max(0.0, x); }
  static double derivative(const double x) { return x > 0 ? 1.0 : 0.0; }
};

struct Identity {
  static double activate(const double x) { return x; }
  static double derivative(const double x) { return 1.0; }
};

struct Sigmoid {
  static double activate(const double x) { return 1.0 / (1.0 + std::exp(-x)); }
  static double derivative(const double x) {
    const double y = activate(x);
    return y * (1.0 - y);
  }
};

struct Tanh {
  static double activate(const double x) { return std::tanh(x); }
  static double derivative(const double x) {
    const double y = activate(x);
    return 1.0 - y * y;
  }
};

// ---------- Softmax 定义（针对向量输入） ----------
struct Softmax {
  static std::vector<double> activate(const std::vector<double> &input) {
    std::vector<double> output(input.size());

    // 数值稳定性：减去最大值
    double max_val = *std::ranges::max_element(input);

    std::vector<double> exp_vals(input.size());
    std::ranges::transform(input, exp_vals.begin(),
                           [max_val](const double x) { return std::exp(x - max_val); });

    double sum_exp = std::accumulate(exp_vals.begin(), exp_vals.end(), 0.0);

    std::ranges::transform(exp_vals, output.begin(),
                           [sum_exp](const double val) { return val / sum_exp; });

    return output;
  }
};

enum class ActivationType { ReLU, Sigmoid, Tanh, Identity };

inline double activate(const double x, const ActivationType type) {
  switch (type) {
  case ActivationType::ReLU:
    return ReLU::activate(x);
  case ActivationType::Sigmoid:
    return Sigmoid::activate(x);
  case ActivationType::Tanh:
    return Tanh::activate(x);
  default:
    return x; // identity fallback
  }
}

inline double derivative(const double x, const ActivationType type) {
  switch (type) {
  case ActivationType::ReLU:
    return ReLU::derivative(x);
  case ActivationType::Sigmoid:
    return Sigmoid::derivative(x);
  case ActivationType::Tanh:
    return Tanh::derivative(x);
  default:
    return 1.0;
  }
}

#endif // ACTIVATION_H
