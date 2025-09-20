/*
 * Created by Zhen Chen on 2025/6/4.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */
#include "../utils/matplotlibcpp.h"
#include <vector>


namespace plt = matplotlibcpp;

int main() {
  std::vector<double> x, y;

  // 强制使用 TkAgg 后端，避免 Qt6Agg 卡死
  plt::backend("TkAgg");  // 等价于 python 中的命令 matplotlib.use("TkAgg")

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
