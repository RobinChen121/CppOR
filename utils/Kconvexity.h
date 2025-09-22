/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 20/09/2025, 22:09
 * Description:
 *
 */

#ifndef KCONVEXITY_H
#define KCONVEXITY_H
#include <map>
#include <vector>

std::pair<bool, std::array<int, 3>> check_K_convexity(const std::map<int, double> &Gy,
                                                      double fix_hire_cost);
#endif // KCONVEXITY_H
