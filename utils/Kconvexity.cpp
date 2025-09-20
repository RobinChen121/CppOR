/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 20/09/2025, 22:09
 * Description:
 *
 */

#include "Kconvexity.h"
#include <iostream>

bool check_K_convexity(const std::vector<double> &Gy, const double fix_hire_cost,
                       const double min_y, const double max_y) {
  for (int y_minus_b = min_y; y_minus_b <= max_y; y_minus_b++)
    for (int y = y_minus_b + 1; y <= max_y; y++)
      for (int y_plus_a = y + 1; y_plus_a <= max_y; y_plus_a++) {
        const double left =
            (y - y_minus_b) * (Gy[y_plus_a - min_y] - Gy[y - min_y] + fix_hire_cost);
        const double right = (y_plus_a - y) * (Gy[y - min_y] - Gy[y_minus_b - min_y]);
        if (left >= right - 0.5)
          continue;
        std::cout << "**************" << std::endl;
        std::cout << "not K convex" << std::endl;
        std::cout << "y - b = " << y_minus_b << ", y = " << y << ", y + a = " << y_plus_a
                  << std::endl;
        std::cout << "left = " << left << ", right = " << right << std::endl;
        return false;
      }
  std::cout << "K convexity holds!" << std::endl;
  return true;
}