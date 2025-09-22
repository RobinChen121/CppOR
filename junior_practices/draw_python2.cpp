/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 20/09/2025, 12:45
 * Description:
 *
 */
#include "../utils/matplotlibcpp.h"
#include <numbers> // C++20
#include <vector>

namespace plt = matplotlibcpp;

int main() {
  int n = 1000;
  std::vector<double> x(n), y(n), z(n);
  plt::backend("TkAgg");

  // 准备数据
  for (int i = 0; i < n; ++i) {
    x[i] = i * i;
    y[i] = sin(2 * std::numbers::pi / 360.0);
    z[i] = log(i + 1); // 避免 log(0)
  }

  // 绘制初始图
  plt::plot(x, y);
  plt::named_plot("log(x)", x, z);
  plt::xlim(0, n * n);
  plt::title("Sample Animation");
  plt::legend();

  // 动画循环：更新数据并暂停显示
  for (int j = 0; j < n; ++j) {
    // 更新 y 数据（示例：动态正弦波）
    for (size_t i = 0; i < y.size(); ++i) {
      y[i] = sin(2 * std::numbers::pi * (i + j) / 360.0);
    }
    plt::clf(); // 清空当前图（可选，避免重叠）
    plt::plot(x, y);
    plt::named_plot("log(x)", x, z);
    plt::pause(0.01); // 暂停 10ms 更新帧
  }

  plt::show(); // 阻塞显示最终图
  return 0;
}
