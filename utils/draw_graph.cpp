/*
 * Created by Zhen Chen on 2025/6/15.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "draw_graph.h"
#include <string>

void drawGy(const std::map<int, double> &arr, const int min_x, const int max_x,
            const double fix_cost, const int capacity) {
  plt::backend("TkAgg");
  std::vector<double> x, y;
  for (int i = min_x; i <= max_x; ++i) {
    x.push_back(i);
    y.push_back(
        arr.at(i)); // at 只读访问或保证 key 存在时安全使用,而直接用[]则在没有key时自动插入value 0
  }
  int S = 0;
  double GS = std::numeric_limits<int>::max();
  for (auto [fst, snd] : arr) {
    if (snd < GS) {
      GS = snd;
      S = fst;
    }
  }
  int s = 0;
  for (int i = min_x; i < S; ++i) {
    if (arr.at(i) < GS + fix_cost) {
      s = i;
      break;
    }
  }

  constexpr double y_max = 3000; // may set according to problem
  constexpr double y_min = -1;   // may set according to problem
  constexpr double y_scale = y_max - y_min;
  plt::ylim(y_min, y_max);

  const std::vector scatter_x = {s, S};
  const std::vector scatter_y = {arr.at(s), GS};
  plt::scatter(scatter_x, scatter_y, 5.0, {{"color", "red"}});
  plt::plot(x, y);
  const std::string title = "G(y): s = " + std::to_string(s) + ", S = " + std::to_string(S) +
                            ", C = " + std::to_string(capacity);
  plt::title(title);

  std::vector<double> x2, y2;
  for (int i = s; i <= s + capacity; ++i) {
    x2.push_back(i);
    double value = arr.at(s);
    y2.push_back(value);
  }
  plt::plot(x2, y2, {{"color", "red"}, {"label", "capacity length"}});

  auto [fst, snd] = check_K_convexity(arr, fix_cost);
  const std::string Kconvexity = fst ? "K-convex" : "not K-convex";
  const int x_scale = max_x + min_x;
  // auto y_lim = plt::ylim(); // 这个ylim()函数会有越界错误,应该是 matplotlibcpp 本身的错误
  plt::text(0.3 * x_scale, 0.9 * y_scale, Kconvexity);

  if (!fst) {
    auto [yb, yy, ya] = snd;
    std::vector<double> x1, y1;
    for (int i = yb; i <= ya; ++i) {
      x1.push_back(i);
      double value = arr.at(yb) + (i - yb) * (arr.at(yy) - arr.at(yb)) / (yy - yb);
      y1.push_back(value);
    }
    plt::plot(x1, y1, {{"color", "green"}, {"label", "not K-convex line"}});

    const std::vector scatter1_x = {ya};
    const std::vector scatter1_y = {arr.at(ya) + fix_cost};
    plt::scatter(scatter1_x, scatter1_y, 10.0, {{"color", "red"}});
  }

  plt::legend(); // 显示图例
  plt::grid(true);
  plt::show();
}

void drawGy(const std::vector<double> &arr, const std::array<int, 2> &arr_sS) {
  plt::backend("TkAgg");
  std::vector<double> x, y;
  for (int i = 0; i < arr.size() - 200; ++i) {
    x.push_back(i);
    y.push_back(arr[i]);
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
  const double GS = y[arr_sS[1]];
  const std::string str = std::to_string(GS);
  // const std::string str = fmt::format("{:.2f}", GS);
  const std::string title = "G(y): s = " + std::to_string(arr_sS[0]) +
                            ", S = " + std::to_string(arr_sS[1]) + ", G(S) = " + str;
  plt::title(title);
  plt::grid(true);
  plt::show();
}

void drawGyAnimation(const std::vector<std::vector<double>> &arr,
                     const std::vector<std::string> &parameter,
                     const std::vector<std::string> &kconvexity,
                     const std::vector<std::string> &binomial_kconvexity,
                     const std::vector<std::string> &convexity) {
  plt::backend("TkAgg");
  int repeat = 3;
  while (repeat > 0) {
    for (int i = 0; i < arr.size(); i++) {
      std::vector<double> x, y;
      for (int j = 0; j < arr[i].size() - 200; ++j) {
        x.push_back(j);
        y.push_back(arr[i][j]);
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
      plt::text(0.35 * x_scale, 0.929 * y_scale, binomial_kconvexity[i]);
      plt::text(0.35 * x_scale, 0.859 * y_scale, convexity[i]);
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