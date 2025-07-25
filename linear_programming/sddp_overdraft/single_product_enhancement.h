/*
 * Created by Zhen Chen on 2025/4/12.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef SINGLE_PRODUCT_ENHANCEMENT_H
#define SINGLE_PRODUCT_ENHANCEMENT_H

#include "../../utils/common.h"
#include "../../utils/sampling.h"
#include "gurobi_c++.h"
#include <vector>

#include <array>
#include <iomanip> // for precision
#include <numeric>
#include <unordered_set>

class SingleProduct {
private:
  // problem settings
  double iniI = 0;
  double iniCash = 0;
  std::vector<double> mean_demands = {15.0, 15.0, 15.0, 15.0}; // std::vector<double>(4, 15);
  std::string distribution_name = "poisson";
  size_t T = mean_demands.size();
  std::vector<double> unit_vari_costs = std::vector<double>(T, 1);
  std::vector<double> prices = std::vector<double>(T, 10);
  double unit_salvage_value = 0.5;
  std::vector<double> overhead_costs = std::vector<double>(T, 50);
  double r0 = 0; // interest rate
  double r1 = 0.1;
  double r2 = 2;
  double overdraftLimit = 500;

  // sddp settings
  int sampleNum = 10;
  int forwardNum = 10;
  int iterNum = 100;
  double thetaInitialValue = -500;

public:
  SingleProduct() {};
  SingleProduct(const std::vector<double> &mean_demands, const double price, const double r1,
                const double overhead_cost, const int sampleNum, const int forwardNum,
                const int iterNum)
      : mean_demands(mean_demands), r1(r1), sampleNum(sampleNum), forwardNum(forwardNum),
        iterNum(iterNum) {
    prices = std::vector(T, price);
    overhead_costs = std::vector(T, overhead_cost);
  };
  [[nodiscard]] std::array<double, 2> solve() const;
};

#endif // SINGLE_PRODUCT_ENHANCEMENT_H
