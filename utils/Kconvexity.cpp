/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 20/09/2025, 22:09
 * Description:
 *
 */

#include "Kconvexity.h"

#include <array>
#include <iostream>

std::pair<bool, std::array<int, 3>> check_K_convexity(const std::map<int, double> &Gy,
                                                      const double fix_hire_cost) {
  const auto first = Gy.begin();
  const int min_y = first->first;
  const auto last = std::prev(Gy.end());
  const int max_y = last->first;
  double max_gap = 0.0;
  int yb = 0, yy = 0, ya = 0;
  for (int y_minus_b = min_y; y_minus_b <= max_y; y_minus_b++)
    for (int y = y_minus_b; y <= max_y; y++)
      for (int y_plus_a = y; y_plus_a <= max_y; y_plus_a++) {
        const double left = (y - y_minus_b) * (Gy.at(y_plus_a) - Gy.at(y) + fix_hire_cost);
        const double right = (y_plus_a - y) * (Gy.at(y) - Gy.at(y_minus_b));
        if (left >= right - 1e-1)
          continue;
        if (right - left > max_gap) {
          max_gap = right - left;
          ya = y_plus_a;
          yb = y_minus_b;
          yy = y;
        }
      }
  std::pair<bool, std::array<int, 3>> result;
  if (max_gap > 1e-1) {
    std::cout << "**************" << std::endl;
    std::cout << "not K convex!" << std::endl;
    std::cout << "y - b = " << yb << ", y = " << yy << ", y + a = " << ya << std::endl;
    std::cout << "k convexity left-right max gap = " << max_gap << std::endl;
    // std::array 和普通数组不一样，不能只靠 {} 让编译器总是自动推导出来
    result = {false, std::array{yb, yy, ya}};
  } else {
    std::cout << "**************" << std::endl;
    std::cout << "K convexity holds!" << std::endl;
    result = {true, std::array{yb, yy, ya}};
  }
  return result;
}