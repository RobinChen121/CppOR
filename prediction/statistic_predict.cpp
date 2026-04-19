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

std::vector<double> Predictor::singleExponentialSmooth(const double alpha,
                                                       const int forecast_step) const {
  const int n = data.size();
  std::vector<double> smooth(n + 1);

  // 初始化：常见做法是 S0 = y0
  smooth[0] = data[0];

  // 平滑过程
  for (int t = 1; t <= n; ++t) {
    smooth[t] = alpha * data[t - 1] + (1 - alpha) * smooth[t - 1];
  }

  // 预测结果（长度 = 历史 + forecast_step）
  std::vector<double> result = smooth;

  const double last_s = smooth.back();

  // 未来预测：SES 的预测是常数（= 最后一个平滑值）
  for (int i = 0; i < forecast_step; ++i) {
    result.push_back(last_s);
  }

  return result;
}

int main() {
  std::vector<double> data = {10, 12, 13, 12, 14, 16};

  const Predictor model(data);

  auto result = model.singleExponentialSmooth(0.3, 3);

  for (const double v : result) {
    std::cout << v << " ";
  }

  return 0;
}