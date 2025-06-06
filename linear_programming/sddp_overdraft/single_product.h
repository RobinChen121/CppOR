/*
 * Created by Zhen Chen on 2025/4/19.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef SINGLE_PRODUCT_H
#define SINGLE_PRODUCT_H

#include "../../utils/sampling.h"
#include "gurobi_c++.h"

#include <iomanip> // for precision
#include <iostream>
// #include <unordered_set>
#include <numeric> // for accumulate
#include <vector>

class SingleProduct {
private:
  // problem settings
  double iniI = 0;
  double iniCash = 0;
  std::vector<double> mean_demands = {15, 15, 15, 15}; // std::vector<double>(4, 15);
  std::string distribution_name = "poisson";
  size_t T = mean_demands.size();
  std::vector<double> unitVariOderCosts = std::vector<double>(T, 1);
  std::vector<double> prices = std::vector<double>(T, 10);
  double unitSalvageValue = 0.5;
  std::vector<double> overhead_costs = std::vector<double>(T, 50);
  double r0 = 0; // interest rate
  double r1 = 0.1;
  double r2 = 2;
  double overdraftLimit = 500;

  // sddp settings
  int sampleNum = 10;  // 10;
  int forwardNum = 30; // 20;
  int iterNum = 50;
  double thetaInitialValue = -500;

public:
  SingleProduct(const std::vector<double> &mean_demands, const double price, const double r1,
                const double overhead_cost, const int sampleNum, const int forwardNum,
                const int iterNum)
      : mean_demands(mean_demands), r1(r1), sampleNum(sampleNum), forwardNum(forwardNum),
        iterNum(iterNum) {
    prices = std::vector(T, price);
    overhead_costs = std::vector(T, overhead_cost);
  };
  SingleProduct() {};
  std::array<double, 2> solve() const;
};

#endif // SINGLE_PRODUCT_H
