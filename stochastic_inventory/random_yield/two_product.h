/*
 * Created by Zhen Chen on 2026/7/31.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef PREDICTION_TWO_PRODUCT_H
#define PREDICTION_TWO_PRODUCT_H

#include <boost/math/special_functions/bessel.hpp>

struct SkellamDistribution {
  int mu;
  int lambda;

  SkellamDistribution(const int mu, const int lambda) : mu(mu), lambda(lambda) {}

  // 概率质量函数 (PMF)
  double pmf(int k) const {
    if (mu <= 0 || lambda <= 0)
      return 0.0;

    const double abs_k = std::abs(k);
    const double scale = std::pow(mu / lambda, k / 2.0);
    const double bessel_val = boost::math::cyl_bessel_i(abs_k, 2.0 * std::sqrt(mu * lambda));

    return std::exp(-(mu + lambda)) * scale * bessel_val;
  }
};

class TwoProduct {
  double h1 = 1.0; // unit holding cost
  double h2 = 2.0;
  double p1 = 2.0; // unit stock out cost
  double p2 = 3.0;

  double K = 50.0; // fixed launching cost
  int mu = 10;     // yielding rate, follows Poisson distribution
  int lambda1 = 5; // demand rate, follows Poisson distribution
  int lambda2 = 8;

  int T = 4;

public:
  TwoProduct() = default;
};

#endif // PREDICTION_TWO_PRODUCT_H
