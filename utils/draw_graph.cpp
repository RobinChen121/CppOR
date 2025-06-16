/*
 * Created by Zhen Chen on 2025/6/15.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "draw_graph.h"

void drawGy(const std::vector<std::array<double, 2>> &arr, const std::array<int, 2> arr_sS) {
  std::vector<double> x, y;
  for (auto item : arr) {
    x.push_back(item[0]);
    y.push_back(item[1]);
  }
  plt::plot(x, y);
  // std::vector<double> s, S;
  // for (const auto item : arr_sS) {
  //   s.push_back(item);
  //   S.push_back(y[item]);
  // }
  const double y_max = y.back();
  const double y_min = y[arr_sS[1]] - 500;
  const double y_scale = y_max - y_min;
  plt::ylim(y_min, y_max);
  plt::axvline(arr_sS[0], 0, (y[arr_sS[0]] - y_min) / y_scale,
               {{"color", "red"}, {"linestyle", "--"}});
  plt::axvline(arr_sS[1], 0, (y[arr_sS[1]] - y_min) / y_scale,
               {{"color", "red"}, {"linestyle", "--"}});
  // plt::text(static_cast<double>(arr_sS[0]), y_min, "s");
  // plt::text(static_cast<double>(arr_sS[1]), y_min, "S");
  double GS = y[arr_sS[1]];
  const std::string str = std::format("{:.2f}", GS);
  const std::string title = "G(y): s = " + std::to_string(arr_sS[0]) +
                            ", S = " + std::to_string(arr_sS[1]) + ", G(S) = " + str;
  plt::title(title);
  plt::grid(true);
  plt::show();
}

// int main() {
//   std::vector<double> x, y;
//
//   for (double i = -10; i <= 10; i += 0.1) {
//     x.push_back(i);
//     // y.push_back(i * i);  // y = x^2
//     y.push_back(std::sin(i)); // y = sin(x)
//   }
//
//   plt::plot(x, y);
//   plt::title("y = sin(x)");
//   plt::xlabel("x");
//   plt::ylabel("y");
//   plt::grid(true);
//   plt::show();
//
//   return 0;
// }