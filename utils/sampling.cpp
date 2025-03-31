/*
 * Created by Zhen Chen on 2025/3/13.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "../utils/sampling.h"
#include <csignal>
#include <random>
#include <ranges>
#include <vector>

// get cumulative distribution function value of Poisson
double poissonCDF(const int k, const double lambda) {
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
int poissonQuantile(const double p, const double lambda) {
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

// 一个 const 函数 通常指的是一个 成员函数，它承诺不会修改类的成员数据
std::vector<double> generateSamplesPoisson(const int num_samples,
                                           const double mean) {
  std::vector<double> samples(num_samples);
  // 创建随机数引擎
  std::random_device rd;  // 随机设备，用来生成随机种子
  std::mt19937 gen(rd()); // 使用 Mersenne Twister 算法的随机数生成器

  for (int i = 0; i < num_samples; ++i) {
    // 创建均匀分布, LHS sampling
    std::uniform_real_distribution<> dis(static_cast<double>(i) / num_samples,
                                         (i + 1.0) / num_samples);
    const double random_value = dis(gen);
    samples[i] = poissonQuantile(random_value, mean);
  }
  return samples;
}

std::vector<double>
generateSamplesSelfDiscrete(const int num_samples, std::vector<double> &weights,
                            const std::vector<double> &values) {
  std::random_device rd;
  std::mt19937 gen(rd());

  std::discrete_distribution<int> dist(weights.begin(), weights.end());
  std::vector<double> samples(num_samples);

  for (int i = 0; i < num_samples; ++i) {
    const int index = dist(gen);
    samples[i] = values[index];
  }

  return samples;
}

int randInt(const int a, const int b) {
  // 创建随机数引擎（使用随机设备作为种子）
  std::random_device rd;
  std::mt19937 gen(rd()); // 使用 Mersenne Twister 作为随机数引擎

  // 定义均匀分布的范围 [a, b]
  std::uniform_int_distribution<int> dist(a, b);

  // 生成并打印一个随机整数
  const int random_number = dist(gen);
  return random_number;
}

std::vector<std::vector<int>>
generateScenarioPaths(const int scenarioNum,
                      const std::vector<int> &num_sampless) {
  const size_t T = num_sampless.size();
  std::vector<std::vector<int>> scenarioPaths(scenarioNum);
  for (int i = 0; i < scenarioNum; ++i) {
    scenarioPaths[i].resize(T);
  }

  for (int i = 0; i < scenarioNum; ++i) {
    for (int t = 0; t < T; ++t) {
      scenarioPaths[i][t] = randInt(0, num_sampless[t] - 1);
    }
  }
  return scenarioPaths;
}

template <typename T>
void print2DArray(const std::vector<std::vector<T>> &arr) {
  for (const auto &row : arr) {
    for (const int num : row) {
      std::cout << num << " ";
    }
    std::cout << std::endl;
  }
}

// 按照所有列依次排序
void sortRowsByAllColumns(std::vector<std::vector<int>> &matrix) {
  std::ranges::sort(matrix,
                    [](const std::vector<int> &a, const std::vector<int> &b) {
                      for (size_t i = 0; i < a.size(); ++i) {
                        if (a[i] != b[i])
                          return a[i] < b[i]; // 按列顺序排序
                      }
                      return false; // 所有列都相等
                    });
}

// int main() {
//   std::vector<double> weights = {0.25, 0.5, 0.5};
//   const std::vector<double> values = {10, 15, 20};
//   constexpr int num_samples = 10;
//   const auto samples =
//       generateSamplesSelfDiscrete(num_samples, weights, values);
//   for (int i = 0; i < num_samples; ++i) {
//     std::cout << samples[i] << " ";
//   }
//
//   return 0;
// }
