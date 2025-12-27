/*
 * Created by Zhen Chen on 2025/3/12.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "cash_state.h"

#include <iostream>

double CashState::get_ini_cash() const { return initial_cash; }

std::ostream &operator<<(std::ostream &os, const CashState &state) {
  os << "Period: " << state.get_period() << ", ini inventory: " << state.get_ini_inventory()
     << ", ini cash: " << state.get_ini_cash() << std::endl;
  return os;
}

// int main() {
//     const CashState state(1, 0, 10);
//     std::cout << state << std::endl;
//     return 0;
// }
