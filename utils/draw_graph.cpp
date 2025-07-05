/*
 * Created by Zhen Chen on 2025/6/15.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "draw_graph.h"
// #include <fmt/core.h>

#include <boost/container/container_fwd.hpp>

void drawGy(const std::vector<std::array<double, 2>> &arr, const std::array<int, 2> &arr_sS) {
  std::vector<double> x, y;
  for (int i = 0; i < arr.size() - 200; ++i) {
    x.push_back(arr[i][0]);
    y.push_back(arr[i][1]);
  }
  plt::plot(x, y);
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
  const std::string str = std::to_string(GS);
  // const std::string str = fmt::format("{:.2f}", GS);
  const std::string title = "G(y): s = " + std::to_string(arr_sS[0]) +
                            ", S = " + std::to_string(arr_sS[1]) + ", G(S) = " + str;
  plt::title(title);
  plt::grid(true);
  plt::show();
}

void drawGyAnimation(const std::vector<std::vector<std::array<double, 2>>> &arr,
                     const std::vector<std::string> &parameter,
                     const std::vector<std::string> &kconvexity) {
  int repeat = 3;
  while (repeat > 0) {
    for (int i = 0; i < arr.size(); i++) {
      std::vector<double> x, y;
      for (int j = 0; j < arr[i].size() - 200; ++j) {
        x.push_back(arr[i][j][0]);
        y.push_back(arr[i][j][1]);
      }
      plt::plot(x, y);
      const double x_min = 0;
      const double x_max = x.back();
      const double y_max = 8000; // y.back();
      const double y_min = 1000; // y[arr_sS[i][1]];
      const double y_scale = y_max - y_min;
      const double x_scale = x_max - x_min;
      plt::ylim(y_min, y_max);
      plt::xlim(x_min, x_max);
      // plt::axvline(arr_sS[i][0], 0, (y[arr_sS[i][0]] - y_min) / y_scale,
      //              {{"color", "red"}, {"linestyle", "--"}});
      // plt::axvline(arr_sS[i][1], 0, (y[arr_sS[i][1]] - y_min) / y_scale,
      //              {{"color", "red"}, {"linestyle", "--"}});
      // double GS = y[arr_sS[i][1]];
      // std::string str = std::format("{:.2f}", GS);
      // const std::string title = "G(y): s = " + std::to_string(arr_sS[i][0]) +
      //                           ", S = " + std::to_string(arr_sS[i][1]) + ", G(S) = " + str;
      // plt::title(title);

      plt::title(parameter[i]);
      plt::text(0.35 * x_scale, 0.999 * y_scale, kconvexity[i]);
      plt::grid(true);
      plt::pause(1);
      plt::clf();
    }
    repeat--;
  }
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