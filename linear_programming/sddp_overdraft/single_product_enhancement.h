/*
 * Created by Zhen Chen on 2025/4/12.
 * Email: chen.zhen5526@gmail.com
 * Description: 
 *
 *
 */

#ifndef SINGLE_PRODUCT_ENHANCEMENT_H
#define SINGLE_PRODUCT_ENHANCEMENT_H

#include<vector>

class SingleProduct {
private:
  // problem settings
  double iniI = 0;
  double iniCash = 0;
  std::vector<double> mean_demands = {
    7, 12, 17, 23}; // {15.0, 15.0, 15.0, 15.0}; // std::vector<double>(4, 15);
  std::string distribution_name = "poisson";
  size_t T = mean_demands.size();
  std::vector<double> unit_vari_costs = std::vector<double>(T, 1);
  std::vector<double> prices = std::vector<double>(T, 10);
  double unit_salvage_value = 0.5;
  std::vector<double> overhead_costs = std::vector<double>(T, 25);
  double r0 = 0; // interest rate
  double r1 = 0;
  double r2 = 2;
  double overdraftLimit = 300;

  // sddp settings
  int sampleNum = 10;
  int forwardNum = 30;
  int iterNum = 50;
  double thetaInitialValue = -500;

public:
  SingleProduct(std::vector<double> mean_demands, double price, double r1, int sampleNum,
                int forwardNum, int iterNum)
      : mean_demands(mean_demands), r1(r1), sampleNum(sampleNum), forwardNum(forwardNum),
        iterNum(iterNum) {
    prices = std::vector<double>(T, price);
  };
  SingleProduct();
  void solve() const;
};

#endif //SINGLE_PRODUCT_ENHANCEMENT_H
