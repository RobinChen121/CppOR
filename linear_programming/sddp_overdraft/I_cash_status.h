/*
 * Created by Zhen Chen on 2025/3/18.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef I_CASH_STATUS_H
#define I_CASH_STATUS_H

#include <array>
#include <boost/functional/hash.hpp>
#include <unordered_map>
#include <vector>

enum class IStatus { // enum class 相对于 class 更安全，有作用域
  POSITIVE,
  NEGATIVE
};

enum class CashStatus { ATW0, ATW1, ATW2 };

// using 是用来给 已存在的类型 起别名的，它不能在 using 的同时定义一个新类型
using PairStatus = std::pair<IStatus, CashStatus>;

using DoubleIStatus = std::pair<IStatus, IStatus>; // pair does not need writing operator==

struct TripleStatus { // using 用来给变量起别名，typedef 的替代品
  IStatus I_status1;
  IStatus I_status2;
  CashStatus cash_status;

  bool operator==(const TripleStatus &other) const {
    return I_status1 == other.I_status1 && I_status2 == other.I_status2 &&
           cash_status == other.cash_status;
  }
};

// 在 C++ 标准库中，std::pair<T1, T2> 已经内置了 operator==，不需要手动重载它
// 自定义哈希函数
// 使用 std::unordered_set 或 std::unordered_map 时需要重载 operator== 和 hash 函数
template <> struct std::hash<PairStatus> {
  std::size_t operator()(const PairStatus &p) const {
    const size_t first_hash = std::hash<IStatus>{}(p.first);
    const size_t second_hash = std::hash<CashStatus>{}(p.second);
    return first_hash ^ (second_hash << 1); // 使用异或和位移合并哈希值
  }
};

template <> struct std::hash<DoubleIStatus> {
  size_t operator()(const DoubleIStatus &p) const {
    // 哈希计算：使用组合哈希
    const size_t first_hash = hash<int>()(static_cast<int>(p.first));
    const size_t second_hash = hash<int>()(static_cast<int>(p.second));
    return first_hash ^ (second_hash << 1); // 使用异或和位移合并哈希值
  }
};

// hash 特化
template <> struct std::hash<TripleStatus> {
  std::size_t operator()(const TripleStatus &t) const {
    std::size_t seed = 0;
    boost::hash_combine(seed, t.I_status1);
    boost::hash_combine(seed, t.I_status2);
    boost::hash_combine(seed, t.cash_status);
    return seed;
  }
};

// self defined hash for pair enum key hashmap
// 这个 hash map 只能局部使用，即必须在定义 map
// 时指定这个hash，不像上一个全局重载 struct pair_hash {
//   template <typename T1, typename T2>
//   std::size_t operator()(const std::pair<T1, T2> &p) const {
//     return std::hash<int>{}(static_cast<int>(p.first)) ^
//            (std::hash<int>{}(static_cast<int>(p.second)) << 1);
//   }
// };

PairStatus check_pair_status(double end_inventory, double end_cash, double overdraft_limit);
TripleStatus checkTripleStatus(double end_inventory1, double end_inventory2, double end_cash,
                               double overdraft_limit);
DoubleIStatus checkDoubleIStatus(double end_inventory1, double end_inventory2);
#endif // COMMON_H
