/*
 * Created by Zhen Chen on 2025/6/15.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef DRAW_GRAPH_H
#define DRAW_GRAPH_H

#include "../utils/matplotlibcpp.h"
#include "Kconvexity.h"
#include <vector>
namespace plt = matplotlibcpp;

void drawGy(const std::vector<double> &arr, const std::array<int, 2> &arr_sS);
void drawGy(const std::map<int, double> &arr, int min_x, int max_x, double fix_cost, int capacity);

void drawGyAnimation(const std::vector<std::map<int, double>> &arr, int min_x, int max_x,
                     double fix_cost, const std::vector<double> &capacities);
void drawGyAnimation(const std::vector<std::vector<double>> &arr,
                     const std::vector<std::string> &parameter,
                     const std::vector<std::string> &kconvexity,
                     const std::vector<std::string> &binomial_kconvexity,
                     const std::vector<std::string> &convexity);

#endif // DRAW_GRAPH_H
