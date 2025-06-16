/*
 * Created by Zhen Chen on 2025/6/16.
 * Email: chen.zhen5526@gmail.com
 * Description: 
 *
 *
 */
#include "../utils/matplotlibcpp.h"
#include <vector>
#include <thread>
#include <chrono>

namespace plt = matplotlibcpp;

int main() {
  std::vector<double> x, y;

  for (int i = 0; i < 100; ++i) {
    x.push_back(i);
    y.push_back(std::sin(i * 0.1));

    plt::clf();                      // 清除上一帧
    plt::plot(x, y);
    plt::pause(0.05);               // 暂停一会用于刷新图像
  }

  plt::show();  // 最后显示
  return 0;
}
