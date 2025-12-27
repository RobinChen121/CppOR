//
// Created by Zhen Chen on 2025/2/28.
//

#include "state.h"
#include <unordered_map>

#include <iostream>
#include <map>

State::State() = default; // 由编译器生成，行为是“成员逐个默认初始化”

State::State(const int period, const double ini_inventory)
    : period(period), ini_inventory(ini_inventory) {};

double State::get_ini_inventory() const { return ini_inventory; }

int State::get_period() const { return period; }

void State::print() const {
  std::cout << "period: " << period << ", ini I: " << ini_inventory << std::endl;
}

// bool State::operator<(const State &other) const {
//   if (period < other.period) {
//     return true;
//   }
//   if (period == other.period) {
//     if (ini_inventory < other.ini_inventory) {
//       return true;
//     }
//     return false;
//   }
//   return false;
// }

std::ostream &operator<<(std::ostream &os, const State &state) {
  os << "period: " << state.period << ", ini I: " << state.ini_inventory << std::endl;
  return os;
}

// // ✅ 自定义比较器
// struct CompareState {
//     bool operator()(const State &p1, const State &p2) const {
//         if (p1.getPeriod() < p2.getPeriod()) {
//             return true;
//         }
//         if (p1.getPeriod() == p2.getPeriod()) {
//             if (p1.getini_inventory() < p2.getini_inventory()) {
//                 return true;
//             }
//             return false;
//         }
//         return false;
//     }
// };

// int main() {
//     const State State(1, 10);
//     State.print();
//     std::cout << State << std::endl;
// }
