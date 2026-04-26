/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 11/04/2026, 11:18
 * Description:
 *
 */

#include "statistic_predict.h"

#include <iostream>
#include <vector>

#include <emscripten/bind.h>
using namespace emscripten;
// 使用 Emscripten 绑定暴露 Simplex 类和 solve 函数
// 必须注册才能调用
EMSCRIPTEN_BINDINGS(predict_module) {
  // 注册 vector 类型，相当于在js中重新定义了几个数据类型
  register_vector<double>("VectorDouble");

  // 注册类
  class_<Predictor>("Predictor")
      .constructor<std::vector<double>>() // 这个是类的构造函数
      // 注册类里面的函数
      // 前面的字符串名字是 js 里面使用的名字
      .function("singleSmooth", &Predictor::singleSmooth);
}

std::vector<double> Predictor::singleSmooth(const double alpha) const {
  const size_t n = data.size();
  std::vector<double> smooth(n + 1);

  // 初始化：常见做法是 S0 = y0
  smooth[0] = data[0];

  // 平滑过程
  for (int t = 1; t <= n; ++t) {
    smooth[t] = alpha * data[t - 1] + (1 - alpha) * smooth[t - 1];
  }

  return smooth;
}

// int main() {
//   std::vector<double> data = {10, 12, 13, 12, 14, 16};
//
//   const Predictor model(data);
//
//   auto result = model.singleSmooth(0.3);
//
//   for (const double v : result) {
//     std::cout << v << " ";
//   }
//
//   return 0;
// }