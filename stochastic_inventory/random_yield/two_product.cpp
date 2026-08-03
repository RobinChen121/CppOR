/*
 * Created by Zhen Chen on 2026/7/31.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */
#include "two_product.h"

struct jointPMF {
  std::vector<uint8_t> supply_support;
  std::vector<uint8_t> demand1_support;
  std::vector<uint8_t> demand2_support;

  std::vector<float> prob_supply;
  std::vector<float> prob_demand1;
  std::vector<float> prob_demand2;
};

jointPMF getPMF(int supply, int demand1, int demand2, double truncated_quantile);

class TwoProduct {
  float h1 = 1.0; // unit holding cost
  float h2 = 2.0;
  float p1 = 2.0; // unit stock out cost
  float p2 = 3.0;

  uint8_t K = 50;      // fixed launching cost
  uint8_t mu = 10;     // yielding rate, follows Poisson distribution
  uint8_t lambda1 = 5; // demand rate, follows Poisson distribution
  uint8_t lambda2 = 8;

  uint8_t T = 4;

public:
  TwoProduct() = default;
};