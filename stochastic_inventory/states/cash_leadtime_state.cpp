/*
 * Created by Zhen Chen on 2025/3/15.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "cash_leadtime_state.h"

double CashLeadtimeState::get_q_pre() const { return q_pre; }

std::ostream &operator<<(std::ostream &os, const CashLeadtimeState &state) {
  os << "Period: " << state.get_period() << ", ini inventory: " << state.get_ini_inventory()
     << ", ini cash: " << state.get_ini_cash() << ", Q in the last period: " << state.get_q_pre()
     << std::endl;
  return os;
}
