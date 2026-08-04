/*
 * Created by Zhen Chen on 2026/7/31.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */
#include "two_product.h"

struct jointPMF {
  std::vector<int> supply_support;
  std::vector<int> demand1_support;
  std::vector<int> demand2_support;

  std::vector<double> prob_supply;
  std::vector<double> prob_demand1;
  std::vector<double> prob_demand2;
};

jointPMF getPMF(int supply, int demand1, int demand2, double truncated_quantile);

class TwoProduct {
  double h1 = 1.0; // unit holding cost
  double h2 = 2.0;
  double p1 = 2.0; // unit stock out cost
  double p2 = 3.0;

  int K = 50;      // fixed launching cost
  int mu = 10;     // yielding rate, follows Poisson distribution
  int lambda1 = 5; // demand rate, follows Poisson distribution
  int lambda2 = 8;

  int T = 4;

public:
  TwoProduct() = default;
};