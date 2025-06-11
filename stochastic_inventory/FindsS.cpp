/*
 * Created by Zhen Chen on 2025/6/11.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "FindsS.h"

std::vector<int> FindsS::sIndex(std::vector<std::vector<const double>> opt_table) const {
  std::vector<int> index_arr;
  bool ordered_state = false;
  const int column = opt_table[0].size();
  for (size_t i = 0; i < opt_table.size(); i++) {
    if (opt_table[i][column - 1] < maxQ and ordered_state == false)
      ordered_state = true;
    else if (opt_table[i][column - 1] == maxQ and ordered_state == true and
             i != opt_table.size() - 1) {
      ordered_state = false;
      index_arr.push_back(i);
    }
  }
}