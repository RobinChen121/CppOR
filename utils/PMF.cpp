//
// Created by Zhen Chen on 2025/2/27.
//

#include "PMF.h"

#include <algorithm> // for transform
#include <cctype>    // for tolower
#include <cmath>
#include <csignal>
#include <iostream>

// initializing the class
PMF::PMF(const double truncatedQuantile, const double stepSize,
         std::string distributionName)
    : truncatedQuantile(truncatedQuantile), stepSize(stepSize),
      distributionName(std::move(distributionName)) {
  // checkName();
} // std::move for efficiency passing in string and vector

// void PMF::checkName() const {
//   auto name = distributionName;
//   std::ranges::transform(name, name.begin(), ::tolower);
//   if (name != "poisson") {
//     std::cout
//         << " distribution not found or to do next for this distribution\n";
//     raise(-1);
//   }
// }

// get the probability mass function value of Poisson
double PMF::poissonPMF(const int k, const double lambda) {
  if (k < 0 || lambda < 0)
    return 0.0; // 确保参数合法
  if (k == 0 and lambda == 0)
    return 1.0;
  return (std::pow(lambda, k) * std::exp(-lambda)) / std::tgamma(k + 1);
  // tgamma(k+1) is a gamma function, 等同于factorial(k)
}

// get cumulative distribution function value of Poisson
double PMF::poissonCDF(const int k, const double lambda) {
  double cumulative = 0.0;
  double term = std::exp(-lambda);
  for (int i = 0; i <= k; ++i) {
    cumulative += term;
    if (i < k)
      term *= lambda / (i + 1); // 递推计算 P(X=i)
  }

  return cumulative;
}

// get inverse cumulative distribution function value of Poisson
int PMF::poissonQuantile(const double p, const double lambda) {
  int low = 0,
      high = std::max(100, static_cast<int>(lambda * 3)); // 初始搜索区间
  while (low < high) {
    if (const int mid = (low + high) / 2; poissonCDF(mid, lambda) < p) {
      low = mid + 1;
    } else {
      high = mid;
    }
  }
  return low;
}

// get probability mass function values for each period of Poisson
// may be not necessary
std::vector<std::vector<std::vector<double>>>
PMF::getPMF(const std::span<double> demands) const {
  if (distributionName == "poisson") {
    return getPMFPoisson(demands);
  }
  return {};
}

// get probability mass function values for each period of Poisson
std::vector<std::vector<std::vector<double>>>
PMF::getPMFPoisson(const std::span<double> demands) const {
  const auto T = demands.size();
  int supportLB[T];
  int supportUB[T];
  for (int i = 0; i < T; ++i) {
    supportUB[i] = poissonQuantile(truncatedQuantile, demands[i]);
    supportLB[i] = poissonQuantile(1 - truncatedQuantile, demands[i]);
  }
  std::vector<std::vector<std::vector<double>>> pmf(
      T, std::vector<std::vector<double>>());
  for (int t = 0; t < T; ++t) {
    const int demandLength =
        static_cast<int>((supportUB[t] - supportLB[t] + 1) / stepSize);
    pmf[t] =
        std::vector<std::vector<double>>(demandLength, std::vector<double>());
    for (int j = 0; j < demandLength; ++j) {
      pmf[t][j] = std::vector<double>(2);
      pmf[t][j][0] = supportLB[t] + j * stepSize;
      const int demand = static_cast<int>(pmf[t][j][0]);
      pmf[t][j][1] =
          poissonPMF(demand, demands[t]) / (2 * truncatedQuantile - 1);
    }
  }

  return pmf;
}

// get probability mass function values for each period of Poisson of two
// products
std::vector<std::vector<std::vector<double>>>
PMF::getPMFPoissonMulti(const std::span<double> demands1,
                        const std::span<double> demands2) const {
  const auto T = demands1.size();
  int supportLB1[T];
  int supportUB1[T];
  int supportLB2[T];
  int supportUB2[T];
  for (int i = 0; i < T; ++i) {
    supportUB1[i] = poissonQuantile(truncatedQuantile, demands1[i]);
    supportLB1[i] = poissonQuantile(1 - truncatedQuantile, demands1[i]);
    supportUB2[i] = poissonQuantile(truncatedQuantile, demands2[i]);
    supportLB2[i] = poissonQuantile(1 - truncatedQuantile, demands2[i]);
  }
  std::vector pmf(T, std::vector<std::vector<double>>());
  for (int t = 0; t < T; ++t) {
    const int demandLength1 =
        static_cast<int>((supportUB1[t] - supportLB1[t] + 1) / stepSize);
    const int demandLength2 =
        static_cast<int>((supportUB2[t] - supportLB2[t] + 1) / stepSize);
    const int demandLength = demandLength1 * demandLength2;
    pmf[t] = std::vector(demandLength, std::vector<double>());
    int index = 0;
    for (int i = 0; i < demandLength1; ++i) {
      for (int j = 0; j < demandLength2; ++j) {
        pmf[t][index] = std::vector<double>(3);
        pmf[t][index][0] = supportLB1[t] + i * stepSize;
        pmf[t][index][1] = supportLB2[t] + j * stepSize;
        const int demand1 = static_cast<int>(pmf[t][index][0]);
        const int demand2 = static_cast<int>(pmf[t][index][1]);
        const double prob1 =
            poissonPMF(demand1, demands1[t]) / (2 * truncatedQuantile - 1);
        const double prob2 =
            poissonPMF(demand2, demands2[t]) / (2 * truncatedQuantile - 1);
        pmf[t][index][2] = prob1 * prob2;
        index += 1;
      }
    }
  }
  return pmf;
}

std::vector<std::vector<std::vector<double>>> PMF::getPMFSelfDiscreteMulti(
    const std::vector<std::vector<double>> &demand1_values,
    const std::vector<std::vector<double>> &demand1_weights,
    const std::vector<std::vector<double>> &demand2_values,
    const std::vector<std::vector<double>> &demand2_weights) {
  const auto T = demand1_values.size();
  std::vector pmf(T, std::vector<std::vector<double>>());

  for (int t = 0; t < T; ++t) {
    const int demandLength1 = demand1_values[t].size();
    const int demandLength2 = demand1_weights[t].size();
    const int demandLength = demandLength1 * demandLength2;
    pmf[t] =
        std::vector<std::vector<double>>(demandLength, std::vector<double>(3));
    int index = 0;
    for (int i = 0; i < demandLength1; ++i) {
      for (int j = 0; j < demandLength2; ++j) {
        pmf[t][index][0] = demand1_values[t][i];
        pmf[t][index][1] = demand2_values[t][j];
        pmf[t][index][2] = demand1_weights[t][i] * demand2_weights[t][j];
        index += 1;
      }
    }
  }
  return pmf;
}

// int main() {
//     const double lambda = 5.0;
//     const double p = 0.9;
//
//     PMF pmf(0.99, 1, "poisson");
//     const int k = pmf.poissonQuantile(p, lambda);
//     std::cout << "逆CDF (p=" << p << ", λ=" << lambda << ") = " << k <<
//     std::endl;
// }
