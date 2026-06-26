/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 25/06/2026, 21:21
 * Description:
 *
 */

#include <vector>
#include "config.h"

#ifndef CHEN_SOLVER_JS_UTIL_H
#define CHEN_SOLVER_JS_UTIL_H

// 当函数极其短小（如 1~3 行）、逻辑极其简单（如类的 Getter/Setter 函数、简单的数学计算），
// 且在核心循环中被高频调用时，考虑手动加上 inline,
// 使得在每次调用该函数的地方，直接把函数的代码“复制粘贴”过去，而不是通过传统的函数调用机制
// （压栈、跳转、出栈）来执行，提高运行效率
inline bool is_zero(const double x) { return x > -EPS && x < EPS; }

// dot product of two 1-dimension array
static double dot(const std::vector<double> &a, const std::vector<double> &b) {
  double s = 0.0;
  for (size_t i = 0; i < a.size(); ++i)
    s += a[i] * b[i];
  return s;
}

#endif // CHEN_SOLVER_JS_UTIL_H
