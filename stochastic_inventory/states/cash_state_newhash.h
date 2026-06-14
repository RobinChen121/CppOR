/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 13/06/2026, 09:39
 * Description:
 *
 */

#ifndef CHEN_SOLVER_JS_CASH_STATE_NEWHASH_H
#define CHEN_SOLVER_JS_CASH_STATE_NEWHASH_H

#include <boost/functional/hash.hpp>

// 将除法改为乘法
constexpr double INV_EPS = 1e4;
class CashState {
  int period;
  double initial_inventory{};
  double initial_cash{};

public:
  CashState(const int period, const double ini_inventory, const double initial_cash)
      : period(period), initial_inventory(ini_inventory), initial_cash(initial_cash) {};

  [[nodiscard]] double getIniCash() const { return initial_cash; };
  [[nodiscard]] int getPeriod() const { return period; };
  [[nodiscard]] double getIniInventory() const { return initial_inventory; };

  // hashmap must define operator == and a struct to compute hash
  bool operator==(const CashState &other) const {
    // 需要定义 `==`
    // const MyClass &other	保证 other 参数不可修改
    // const 在函数结尾 保证当前对象(this) 不可修改
    // 不会修改成员变量的方法 都可以在函数声明的结尾添加 const
    return getPeriod() == other.getPeriod() &&
           (static_cast<long long>(initial_inventory * INV_EPS) ==
            static_cast<long long>(initial_inventory * INV_EPS)) &&
           (static_cast<long long>(initial_cash * INV_EPS) ==
            static_cast<long long>(other.initial_cash * INV_EPS));
  }
};

// 辅助函数：专门用来组合哈希值
inline void hash_combine(std::size_t &seed, const std::size_t value) {
  seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <> // 表示模版特化， override 标准库中的 hash
            // 生成函数，必须有，否则出错
            // 一般放到头文件，否则出错
struct std::hash<CashState> {
  // size_t 表示无符号整数
  size_t operator()(const CashState &s) const noexcept {
    const auto inv = static_cast<long long>(s.getIniInventory() * INV_EPS);
    const auto cash = static_cast<long long>(s.getIniCash() * INV_EPS);

    size_t seed = 0;

    // 依次将各个字段的哈希值混入 seed 中
    hash_combine(seed, std::hash<int>{}(s.getPeriod()));
    hash_combine(seed, std::hash<long long>{}(inv));
    hash_combine(seed, std::hash<long long>{}(cash)); // 把漏掉的 cash 补上

    return seed;
  }
};

#endif // CHEN_SOLVER_JS_CASH_STATE_NEWHASH_H
