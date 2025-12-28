/*
 * Created by Zhen Chen on 2025/3/21.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "cash_leadtime_multi_state.h"

#include <iostream>
#include <ostream>

bool CashLeadtimeMultiState::operator==(const CashLeadtimeMultiState &other) const {
  return period == other.period && ini_I1 == other.ini_I1 && ini_I2 == other.ini_I2 &&
         q_pre1 == other.q_pre1 && q_pre2 == other.q_pre2 && ini_cash == other.ini_cash;
}

std::ostream &operator<<(std::ostream &os, const CashLeadtimeMultiState &state) {
  os << "Period: " << state.period << ", ini inventory1: " << state.ini_I1
     << ", ini inventory2: " << state.ini_I2 << ", q_pre1: " << state.q_pre1
     << ", q_pre2: " << state.q_pre2 << ", ini cash: " << state.ini_cash << std::endl;
}

// int main() {
//   const CashLeadtimeMultiState state{1, 10, 45, 10, 5, 0};
//   std::cout << state;
//
//   return 0;
// }