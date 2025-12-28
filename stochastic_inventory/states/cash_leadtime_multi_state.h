/*
 * Created by Zhen Chen on 2025/3/21.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef CASHLEADTIMEMULTISTATE_H
#define CASHLEADTIMEMULTISTATE_H

#include <boost/functional/hash.hpp>

class CashLeadtimeMultiState {
private:
  int period{}; // c++11, {} 值初始化，默认为 0
  double ini_I1{};
  double ini_I2{};
  double q_pre1{};
  double q_pre2{};
  double ini_cash{};

public:
  CashLeadtimeMultiState(const int period, const double ini_I1, const double ini_I2,
                         const double q_pre1, const double q_pre2, const double ini_cash)
      : period(period), ini_I1(ini_I1), ini_I2(ini_I2), q_pre1(q_pre1), q_pre2(q_pre2),
        ini_cash(ini_cash) {};

  bool operator==(const CashLeadtimeMultiState &other) const;

  friend std::ostream &operator<<(std::ostream &os, const CashLeadtimeMultiState &state);

  friend struct std::hash<CashLeadtimeMultiState>;

  // [[nodiscard]] 是 C++17 引入的属性，它的作用是
  // 提示调用者不要忽略返回值，否则编译器会给出警告
  [[nodiscard]] int get_period() const { return period; }
  [[nodiscard]] double get_iniI1() const { return ini_I1; }
  [[nodiscard]] double get_iniI2() const { return ini_I2; }
  [[nodiscard]] double get_q_pre1() const { return q_pre1; }
  [[nodiscard]] double get_q_pre2() const { return q_pre2; }
  [[nodiscard]] double get_ini_cash() const { return ini_cash; }
};

template <> // 表示模版特化， override 标准库中的 hash
            // 生成函数，必须有，否则出错
            // 模版特化的函数需要放到头文件，否则出错
struct std::hash<CashLeadtimeMultiState> {
  // size_t 表示无符号整数
  size_t operator()(const CashLeadtimeMultiState &s) const noexcept {
    // noexcept 表示这个函数不会抛出异常
    // boost 的哈希计算更安全
    std::size_t seed = 0;
    boost::hash_combine(seed, s.period);
    boost::hash_combine(seed, s.ini_I1);
    boost::hash_combine(seed, s.ini_I2);
    boost::hash_combine(seed, s.q_pre1);
    boost::hash_combine(seed, s.q_pre2);
    boost::hash_combine(seed, s.ini_cash);
    return seed;
  }
};

#endif // CASHLEADTIMEMULTISTATE_H
