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
#include <cmath>
#include <vector>
namespace plt = matplotlibcpp;

void drawGy(const std::vector<std::array<double, 2>> &arr, std::array<int, 2> arr_sS);

#endif // DRAW_GRAPH_H
