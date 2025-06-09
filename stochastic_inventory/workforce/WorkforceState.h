/*
 * Created by Zhen Chen on 2025/6/3.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef WORKFORCESTATE_H
#define WORKFORCESTATE_H

#include <boost/functional/hash.hpp>

class WorkforceState {
  int period{};
  int initial_workers{};

public:
  WorkforceState() = default; // WorkforceState() {} 并不初始化类的变量
  WorkforceState(const int period, const int initial_workers)
      : period(period), initial_workers(initial_workers) {};

  [[nodiscard]] int getPeriod() const { return period; }
  [[nodiscard]] int get_initial_workers() const { return initial_workers; }

  // define operator < or give a self defined comparator for sorting map
  bool operator<(const WorkforceState &other) const;

  // hashmap must define operator == and a struct to compute hash
  // bool operator == 也可以放在 cpp 文件中，尤其是比较复杂的相等时
  bool operator==(const WorkforceState &other) const {
    // 需要定义 `==`
    // const MyClass &other	保证 other 参数不可修改
    // const 在函数结尾 保证当前对象(this) 不可修改
    // 不会修改成员变量的方法 都可以在函数声明的结尾添加 const
    return period == other.period && initial_workers == other.initial_workers;
  }

  // 允许哈希结构体访问私有成员
  // friend struct
  friend struct std::hash<WorkforceState>;
};

// `std::hash<State>` 需要特化
// 一般放到头文件，否则出错
template <>
// 当你为自定义类（比如 MyClass）提供 std::hash 的特化时，
// 你并不是定义一个全新的类，而是对已有的模板类 std::hash
// 进行特化（specialization）。 C++ 要求特化一个模板时，使用 template<>
// 语法，表示这是一个针对特定类型的特化版本， 而不是一个普通的模板定义。
struct std::hash<WorkforceState> {
  // size_t 表示无符号整数
  size_t operator()(const WorkforceState &s) const noexcept {
    // noexcept 表示这个函数不会抛出异常
    // boost 的哈希计算更安全
    std::size_t seed = 0;
    boost::hash_combine(seed, s.period);
    boost::hash_combine(seed, s.initial_workers);
    return seed;

    // return std::hash<int>()(s.period) ^
    // std::hash<double>()(s.initial_workers) << 1; // 计算哈希值
    // std::hash<int>() 是一个 std::hash<int> 类型的对象，调用 () 运算符可以计算
    // obj.id（整数）的哈希值
    // ^（异或）是位运算，不会造成进位，适合合并多个哈希值
    // 这里的 << 1 左移 1 位（相当于乘
    // 2），让哈希值更加分散，避免简单叠加导致哈希冲突
  }
};

#endif // WORKFORCESTATE_H
