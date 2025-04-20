/*
 * Created by Zhen Chen on 2025/4/19.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef SINGLE_PRODUCT_ENHANCEMENT_FURTHER_H
#define SINGLE_PRODUCT_ENHANCEMENT_FURTHER_H

#include "../../utils/common.h"
#include "../../utils/sampling.h"
#include "I_cash_status.h"
#include "gurobi_c++.h"

#include <array>
#include <fstream>
#include <numeric>
#include <unordered_set>
#include <vector>

class SingleProduct {
private:
  // problem settings
  double iniI = 0;
  double iniCash = 0;
  std::vector<double> mean_demands = {
      7, 12, 17, 23}; // 15.0, 15.0, 15.0, 15.0}; // std::vector<double>(4, 15);
  std::string distribution_name = "poisson";
  size_t T = mean_demands.size();
  std::vector<double> unitVariOderCosts = std::vector<double>(T, 1);
  std::vector<double> prices = std::vector<double>(T, 10);
  double unitSalvageValue = 0.5;
  std::vector<double> overhead_costs = std::vector<double>(T, 25);
  double r0 = 0; // interest rate
  double r1 = 0;
  double r2 = 2;
  double overdraftLimit = 500;

  // sddp settings
  int sampleNum = 10;  // 10; // 2
  int forwardNum = 30; // 20; // 8
  int iterNum = 50;    //
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
  SingleProduct();
  std::array<double, 2> solve() const;
};

#endif // SINGLE_PRODUCT_ENHANCEMENT_FURTHER_H
