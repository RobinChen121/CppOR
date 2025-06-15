/*
 * Created by Zhen Chen on 2025/6/3.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "worker_state.h"
#include <iostream>

bool WorkerState::operator<(const WorkerState &other) const {
  if (period < other.period) {
    return true;
  }
  if (period == other.period) {
    if (initial_workers < other.initial_workers) {
      return true;
    }
    return false;
  }
  return false;
}

// int main() {
//   constexpr auto state =
//       WorkerState(); // constexpr 表示“这个东西（变量或函数）能在编译时就被求值”，用来提升性能
//   std::cout << state.getPeriod() << std::endl;
// }