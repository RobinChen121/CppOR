/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 29/09/2025, 20:53
 * Description:
 *
 */

#ifndef TSP_H
#define TSP_H

#include <climits>
#include <vector>

const std::vector<std::vector<int>> C = {
    {0, 5, 21, 13, 6, 15, 18, 20},   {5, 0, 16, 18, 7, 12, 19, 17}, {21, 16, 0, 33, 16, 7, 17, 11},
    {13, 18, 33, 0, 17, 26, 16, 29}, {6, 7, 16, 17, 0, 9, 12, 14},  {15, 12, 7, 26, 9, 0, 10, 5},
    {18, 19, 17, 16, 12, 10, 0, 13}, {20, 17, 11, 29, 14, 5, 13, 0}};

#endif // TSP_H
