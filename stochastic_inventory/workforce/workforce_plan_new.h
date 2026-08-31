/*
 * Created by Zhen Chen on 2026/8/31.
 * Email: chen.zhen5526@gmail.com
 * Description: use a 1-D vector to store the value function and policy function, which is faster
 * than using unordered_map.
 *
 *
 */

#ifndef WORKFORCE_WORKFORCE_PLAN_NEW_H
#define WORKFORCE_WORKFORCE_PLAN_NEW_H

#include <vector>

class WorkforcePlan {
  std::vector<double> turnover_rates = {0.1, 0.3, 0.5, 0.1, 0.3, 0.5, 0.1, 0.3};
};

#endif // WORKFORCE_WORKFORCE_PLAN_NEW_H
