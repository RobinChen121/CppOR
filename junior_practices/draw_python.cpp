/*
 * Created by Zhen Chen on 2025/6/4.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */
#include "../utils/matplotlibcpp.h"
#include <cmath>
#include <vector>

namespace plt = matplotlibcpp;

int main() {
  std::vector<double> x, y;

  for (double i = -10; i <= 10; i += 0.1) {
    x.push_back(i);
    // y.push_back(i * i);  // y = x^2
    y.push_back(std::sin(i)); // y = sin(x)
  }

  plt::plot(x, y);
  plt::title("y = sin(x)");
  plt::xlabel("x");
  plt::ylabel("y");
  plt::grid(true);
  plt::show();

  return 0;
}
