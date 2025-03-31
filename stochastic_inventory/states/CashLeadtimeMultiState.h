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
  int period;
  double ini_I1;
  double ini_I2;
  double Qpre1;
  double Qpre2;
  double iniCash;

public:
  CashLeadtimeMultiState(const int period, const double ini_I1,
                         const double ini_I2, const double Qpre1,
                         const double Qpre2, const double iniCash)
      : period(period), ini_I1(ini_I1), ini_I2(ini_I2), Qpre1(Qpre1),
        Qpre2(Qpre2), iniCash(iniCash) {};

  bool operator==(const CashLeadtimeMultiState &other) const;

  friend std::ostream &operator<<(std::ostream &os,
                                  const CashLeadtimeMultiState &state);

  friend struct std::hash<CashLeadtimeMultiState>;

  // [[nodiscard]] 是 C++17 引入的属性，它的作用是
  // 提示调用者不要忽略返回值，否则编译器会给出警告
  [[nodiscard]] int getPeriod() const { return period; }
  [[nodiscard]] double getIniI1() const { return ini_I1; }
  [[nodiscard]] double getIniI2() const { return ini_I2; }
  [[nodiscard]] double getQpre1() const { return Qpre1; }
  [[nodiscard]] double getQpre2() const { return Qpre2; }
  [[nodiscard]] double getIniCash() const { return iniCash; }
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
    boost::hash_combine(seed, s.Qpre1);
    boost::hash_combine(seed, s.Qpre2);
    boost::hash_combine(seed, s.iniCash);
    return seed;
  }
};

#endif // CASHLEADTIMEMULTISTATE_H
