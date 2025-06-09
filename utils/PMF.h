//
// Created by Zhen Chen on 2025/2/27.
//
#pragma once

#ifndef PROBABILITYMASSFUNCTIONS_H
#define PROBABILITYMASSFUNCTIONS_H

#include <span>
#include <string>
#include <vector>

class PMF {
  double truncatedQuantile = 1;
  double stepSize = 1;
  std::string distributionName;

public:
  PMF() = default;
  PMF(double truncatedQuantile, double stepSize, std::string distributionName);

  // std::string getName();

  // void checkName() const;

  static double poissonPMF(int k, double lambda);

  // span 本身就是引用传递
  // 普通值参数前面没必要加 const
  // const 的主要目的是代码安全性和可维护性
  // 也有可能提高编译速度
  // 在某些场景（如传引用避免拷贝、 帮助编译器优化）下，确实会带来性能提升
  // [[nodiscard]] std::vector<std::vector<std::vector<double>>>
  // getPMF(std::span<const double> demands) const;

  [[nodiscard]] std::vector<std::vector<std::vector<double>>>
  getPMFPoisson(std::span<const double> demands) const;

  // 函数声明为 static 表示不需要对象初始化就能调用该函数, 不能与 函数的const 同时用
  [[nodiscard]] static std::vector<std::vector<std::vector<double>>>
  getPMFBinomial(int max_staff, std::span<const double> ps);

  static std::vector<std::vector<std::vector<std::array<double, 2>>>>
  getPMFBinomial2(int max_staff, std::span<const double> ps);

  static int poissonQuantile(double p, double lambda);

  static double poissonCDF(int k, double lambda);

  [[nodiscard]] std::vector<std::vector<std::vector<double>>>
  getPMFPoissonMulti(std::span<const double> demands1, std::span<const double> demands2) const;

  static std::vector<std::vector<std::vector<double>>>
  getPMFSelfDiscreteMulti(const std::vector<std::vector<double>> &demand1_values,
                          const std::vector<std::vector<double>> &demand1_weights,
                          const std::vector<std::vector<double>> &demand2_values,
                          const std::vector<std::vector<double>> &demand2_weights);
};

#endif // PROBABILITYMASSFUNCTIONS_H
