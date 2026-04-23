/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 11/04/2026, 11:18
 * Description:
 *
 */

#include "statistic_predict.h"

#include <iostream>
#include <stdexcept>
#include <vector>

std::vector<double> Predictor::singleSmooth(const double alpha) const {
  const int n = data.size();
  std::vector<double> smooth(n + 1);

  // 初始化：常见做法是 S0 = y0
  smooth[0] = data[0];

  // 平滑过程
  for (int t = 1; t <= n; ++t) {
    smooth[t] = alpha * data[t - 1] + (1 - alpha) * smooth[t - 1];
  }

  return smooth;
}

int main() {
  std::vector<double> data = {10, 12, 13, 12, 14, 16};

  const Predictor model(data);

  auto result = model.singleExponentialSmooth(0.3);

  for (const double v : result) {
    std::cout << v << " ";
  }

  return 0;
}