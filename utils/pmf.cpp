//
// Created by Zhen Chen on 2025/2/27.
//

#include "pmf.h"

// #include <algorithm> // for transform
// #include <cctype>    // for tolower
#include <cmath>
// #include <csignal>
#include <boost/math/distributions/binomial.hpp> // 二项分布头文件, random 库有分布但没有pdf函数
#include <boost/math/distributions/normal.hpp>

// initializing the class
PMF::PMF(const double truncated_quantile, const double step_size)
    : truncated_quantile(truncated_quantile), step_size(step_size) {
  // checkName();
} // std::move for efficiency passing in string and vector

// void PMF::checkName() const {
//   auto name = distribution_name;
//   std::ranges::transform(name, name.begin(), ::tolower);
//   if (name != "poisson") {
//     std::cout
//         << " distribution not found or to do next for this distribution\n";
//     raise(-1);
//   }
// }

// get the probability mass function value of Poisson
double PMF::poisson_pmf(const int k, const int lambda) {
  if (k < 0 || lambda < 0)
    return 0.0; // 确保参数合法
  if (k == 0 and lambda == 0)
    return 1.0;
  // lgamma 对 tgamma 取 ln
  const double logP = -lambda + k * std::log(lambda) - std::lgamma(k + 1);
  return std::exp(logP); // Use the logarithmic form to avoid overflow from std::tgamma(k + 1)

  // return (std::pow(lambda, k) * std::exp(-lambda)) / std::tgamma(k + 1);
  // // tgamma(k+1) is a gamma function, 等同于 factorial(k)
}

// get the probability mass function value of Poisson
double PMF::normalPMF(const int k, const double mean, const double sigma,
                      const double truncate_quantile) {
  const auto dist = boost::math::normal_distribution<>(mean, sigma);
  const double pmf = cdf(dist, k + 0.5) - cdf(dist, k - 0.5);
  return pmf / (2 * truncate_quantile - 1);
}

// get cumulative distribution function value of Poisson
double PMF::poisson_cdf(const int k, const double lambda) {
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
int PMF::poisson_quantile(const double p, const double lambda) {
  int low = 0, high = std::max(100, static_cast<int>(lambda * 3)); // 初始搜索区间
  while (low < high) {
    if (const int mid = (low + high) / 2; poisson_cdf(mid, lambda) < p) {
      low = mid + 1;
    } else {
      high = mid;
    }
  }
  return low;
}

// get probability mass function values for each period of self defined distribution
std::vector<std::vector<std::vector<double>>>
PMF::getPMFSelfDefine(const std::span<const double> demands, const std::span<const double> probs,
                      const int T) {
  std::vector pmf(T, std::vector<std::vector<double>>());
  for (int t = 0; t < T; ++t) {
    const int demand_length = demands.size();
    pmf[t].resize(demand_length);
    for (int j = 0; j < demand_length; ++j) {
      pmf[t][j] = std::vector<double>(2);
      pmf[t][j][0] = demands[j];
      pmf[t][j][1] = probs[j];
    }
  }
  return pmf;
}

// get probability mass function values for each period of Poisson
std::vector<std::vector<std::array<double, 2>>>
PMF::get_pmf_poisson(const std::vector<double> &demands) const {
  const size_t T = demands.size();
  std::vector<int> support_lb(T);
  std::vector<int> support_ub(T);
  for (size_t i = 0; i < T; ++i) {
    support_ub[i] = poisson_quantile(truncated_quantile, demands[i]);
    support_lb[i] = poisson_quantile(1 - truncated_quantile, demands[i]);
  }
  std::vector pmf(T, std::vector<std::array<double, 2>>());
  for (int t = 0; t < T; ++t) {
    const int demand_length = static_cast<int>((support_ub[t] - support_lb[t] + 1) / step_size);
    pmf[t].resize(demand_length, std::array<double, 2>());
    for (int j = 0; j < demand_length; ++j) {
      pmf[t][j][0] = support_lb[t] + j * step_size;
      const int demand = static_cast<int>(pmf[t][j][0]);
      pmf[t][j][1] =
          poisson_pmf(demand, static_cast<int>(demands[t])) / (2 * truncated_quantile - 1);
    }
  }
  return pmf;
}

// get probability mass function values for each period of Normal distribution
std::vector<std::vector<std::vector<double>>>
PMF::getPMFNormal(const std::span<const double> mean, const std::span<const double> sigma) const {
  const size_t T = mean.size();

  std::vector<boost::math::normal_distribution<double>> normals(T);
  std::vector<int> support_lb(T);
  std::vector<int> support_ub(T);
  std::vector<double> zero_quantile(T);
  for (size_t t = 0; t < T; ++t) {
    // emplace_back在容器末尾直接构造对象，无需先创建临时对象
    // push_back 把一个已经存在的对象 拷贝或移动 到容器末尾
    normals[t] = boost::math::normal_distribution<>(mean[t], sigma[t]);
    support_ub[t] = static_cast<int>(boost::math::quantile(normals[t], truncated_quantile));
    support_lb[t] =
        0; // static_cast<int>(boost::math::quantile(normals[t], 1 - truncated_quantile));
    zero_quantile[t] = cdf(normals[t], 0.0);
  }

  std::vector pmf(T, std::vector<std::vector<double>>());
  for (int t = 0; t < T; ++t) {
    const int demand_length = static_cast<int>((support_ub[t] - support_lb[t] + 1) / step_size);
    pmf[t] = std::vector(demand_length, std::vector<double>());
    for (int j = 0; j < demand_length; ++j) {
      pmf[t][j] = std::vector<double>(2);
      pmf[t][j][0] = static_cast<int>(support_lb[t] + j * step_size);
      const auto demand = pmf[t][j][0];
      pmf[t][j][1] = (cdf(normals[t], demand + 0.5) - cdf(normals[t], demand - 0.5)) /
                     (truncated_quantile - zero_quantile[t]);
    }
  }
  return pmf;
}

// get probability mass function values for each period of Binomial
// 头文件避免多余 const，保持清晰
// cpp 中加 const 限制修改，提升安全性
std::vector<std::vector<std::vector<double>>>
PMF::getPMFBinomial(const int max_staff, const std::span<const double> ps) {
  const auto T = ps.size();
  std::vector pmf(T, std::vector<std::vector<double>>());
  for (size_t t = 0; t < T; ++t) {
    pmf[t] = std::vector(max_staff + 1, std::vector<double>());
    for (int i = 0; i <= max_staff; ++i) {
      pmf[t][i] = std::vector<double>(i + 1);
      if (i == 0)
        pmf[t][i][0] = 1;
      else {
        boost::math::binomial_distribution<> dist(i, ps[t]);
        for (int j = 0; j <= i; ++j) {
          pmf[t][i][j] = pdf(dist, j);
        }
      }
    }
  }
  return pmf;
}

std::vector<std::vector<std::vector<std::array<double, 2>>>>
PMF::getPMFBinomial2(const int max_staff, const std::span<const double> ps) {
  const auto T = ps.size();
  std::vector pmf(T, std::vector<std::vector<std::array<double, 2>>>());
  for (size_t t = 0; t < T; ++t) {
    pmf[t] = std::vector(max_staff + 1, std::vector<std::array<double, 2>>());
    for (int i = 0; i <= max_staff; ++i) {
      pmf[t][i] = std::vector<std::array<double, 2>>(i + 1);
      if (i == 0) {
        pmf[t][i][0][0] = 0;
        pmf[t][i][0][1] = 1;
      } else {
        boost::math::binomial_distribution<double> dist(i, ps[t]);
        for (int j = 0; j <= i; ++j) {
          pmf[t][i][j][0] = j;
          pmf[t][i][j][1] = pdf(dist, j);
        }
      }
    }
  }
  return pmf;
}

// get probability mass function values for each period of Poisson of two
// products
std::vector<std::vector<std::array<double, 3>>>
PMF::get_pmf_poisson_multi(const std::vector<double> &demands1,
                           const std::vector<double> &demands2) const {
  const auto T = demands1.size();
  std::vector<int> support_lb1(T);
  std::vector<int> support_ub1(T);
  std::vector<int> support_lb2(T);
  std::vector<int> support_ub2(T);
  for (size_t i = 0; i < T; ++i) {
    support_ub1[i] = poisson_quantile(truncated_quantile, demands1[i]);
    support_lb1[i] = poisson_quantile(1 - truncated_quantile, demands1[i]);
    support_ub2[i] = poisson_quantile(truncated_quantile, demands2[i]);
    support_lb2[i] = poisson_quantile(1 - truncated_quantile, demands2[i]);
  }
  std::vector pmf(T, std::vector<std::array<double, 3>>());
  for (size_t t = 0; t < T; ++t) {
    const auto demandLength1 = support_ub1[t] - support_lb1[t] + 1;
    const auto demandLength2 = support_ub2[t] - support_lb2[t] + 1;
    const auto demand_length = demandLength1 * demandLength2;
    pmf[t].resize(demand_length);
    int index = 0;
    for (int i = 0; i < demandLength1; ++i) {
      for (int j = 0; j < demandLength2; ++j) {
        pmf[t][index][0] = support_lb1[t] + i;
        pmf[t][index][1] = support_lb2[t] + j;
        const int demand1 = static_cast<int>(pmf[t][index][0]);
        const int demand2 = static_cast<int>(pmf[t][index][1]);
        const double prob1 =
            poisson_pmf(demand1, static_cast<int>(demands1[t])) / (2 * truncated_quantile - 1);
        const double prob2 =
            poisson_pmf(demand2, static_cast<int>(demands2[t])) / (2 * truncated_quantile - 1);
        pmf[t][index][2] = prob1 * prob2;
        index += 1;
      }
    }
  }
  return pmf;
}

std::vector<std::vector<std::vector<double>>>
PMF::getPMFSelfDiscreteMulti(const std::vector<std::vector<double>> &demand1_values,
                             const std::vector<std::vector<double>> &demand1_weights,
                             const std::vector<std::vector<double>> &demand2_values,
                             const std::vector<std::vector<double>> &demand2_weights) {
  const auto T = demand1_values.size();
  std::vector pmf(T, std::vector<std::vector<double>>());

  for (int t = 0; t < T; ++t) {
    const auto demand_length1 = demand1_values[t].size();
    const auto demand_length2 = demand1_weights[t].size();
    const auto demand_length = demand_length1 * demand_length2;
    pmf[t] = std::vector<std::vector<double>>(demand_length, std::vector<double>(3));
    int index = 0;
    for (int i = 0; i < demand_length1; ++i) {
      for (int j = 0; j < demand_length2; ++j) {
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
//   constexpr double ps[] = {0.5, 0.5};
//
//   auto pmf = PMF::getPMFBinomial(10, ps);
//   std::cout << "test" << std::endl;
// }
