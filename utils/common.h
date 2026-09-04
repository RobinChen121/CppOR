/*
 * Created by Zhen Chen on 2025/3/13.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef COMMON_H
#define COMMON_H
#include <boost/functional/hash.hpp>
#include <fstream> // for file <<
#include <iomanip> // for std::fixed and std::setprecision
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits> // for std::is_floating_point
#include <vector>

using Matrix = std::vector<std::vector<double>>;

Matrix remove_duplicate_rows(const Matrix &mat);

// 自定义哈希函数
struct VectorHash {
  size_t operator()(const std::vector<double> &v) const {
    std::size_t seed = 0;
    for (const double num : v) {
      boost::hash_combine(seed, num);
    }
    return seed;
  }
};

struct VectorEqual {
  bool operator()(const std::vector<double> &a, const std::vector<double> &b) const {
    if (a.size() != b.size())
      return false;
    for (size_t i = 0; i < a.size(); ++i)
      if (std::abs(a[i] - b[i]) > 1e-4)
        return false;
    return true;
  }
};

// C++ 的模板必须在编译时已知它的定义，因此模板函数的实现一般会放在头文件（.h 或
// .hpp）中，而不是源文件（.cpp）
template <typename T1, typename T2>
std::vector<std::pair<T1, T2>> cartesian_product(const std::vector<T1> &a,
                                                 const std::vector<T2> &b) {
  std::vector<std::pair<T1, T2>> result;

  for (const auto &i : a) {
    for (const auto &j : b) {
      result.emplace_back(i, j);
    }
  }
  return result;
}

std::string to_csv_line(const std::vector<std::string> &row);

// 打印一个一维 vector
template <typename T>
std::string vectorToString(const std::vector<T> &vec, const std::string &delimiter = ", ") {
  std::ostringstream
      oss; // 创建一个字符串输出流对象 oss，你可以像写文件一样往里面写东西，它会自动拼成一个字符串

  for (size_t i = 0; i < vec.size(); ++i) {
    if constexpr (std::is_floating_point<T>::value) {
      oss << std::fixed << std::setprecision(2) << vec[i];
    } else {
      oss << vec[i];
    }

    if (i != vec.size() - 1)
      oss << delimiter;
  }

  return oss.str();
}

// 通用写入函数：支持任意类型的 vector，但vector里面类型必须一致
template <typename T>
void append_csv_row(const std::string &filename, const std::vector<T> &row_data);

// 支持任意数据类型转化为字符串的函数模板
// &&arg 表示完美转发，保持参数的值类别（左值或右值）不变
// 辅助转换函数
template <typename T> std::string anyToString(const T &arg) {
  // std::decay_t<T> 是 C++11 引入的类型特性，用于去除类型的引用和 const/volatile 修饰
  // 并将数组或函数退化为指针，从而还原出最基础、最纯粹的原始数据类型
  using DecayedT = std::decay_t<T>;

  if constexpr (std::is_same_v<DecayedT, std::string>) {
    // 1. 如果本身就是 std::string，直接返回（支持完美转发）
    return arg;
  } else if constexpr (std::is_convertible_v<DecayedT, const char *>) {
    // 2. 如果是 const char* 或字符串字面量 (如 "hello")，隐式转为 std::string
    return std::string(arg);
  } else if constexpr (std::is_arithmetic_v<DecayedT>) {
    // 3. 只有当类型是内置数值类型 (int, double, float等) 时，才调用 std::to_string
    return std::to_string(arg);
  } else {
    // 其他自定义类型（只要重载了 operator<<）走 stream 兜底
    std::ostringstream oss;
    oss << std::setprecision(15) << arg; // 补上精度设置
    return oss.str();
  }
}

// 模板函数要放到头文件中，因为编译器需要在编译时看到模板的定义才能生成相应的代码实例化
// 通用变长参数写入函数
// 在 C++ 语法规则中，const T& 具有特殊的“通吃”能力，等价于 T&&
// 它可以绑定到任何类型的对象，包括左值（如变量）、右值（如临时对象）、const 对象、非 const 对象等
template <typename... Args>
void appendCSVRowAny(const std::string &filename,
                     const Args &...args) { // Args &...args 表示零个或多个任意类型的左值引用参数
  std::ofstream file(filename, std::ios::app);
  if (!file.is_open()) {
    std::cerr << "unable to open the file: " << filename << std::endl;
    return;
  }
  // 省略号在包含参数包（如
  // args）的表达式后面时，它的作用是：把前面的函数重复应用到参数包里的每一个参数上，并用逗号隔开展开
  const std::vector<std::string> string_rows = {anyToString(args)...};
  file << to_csv_line(string_rows) << "\n";
  file.close();
}

template <typename T> void print2D(std::vector<std::vector<T>> arr) {
  for (const auto &row : arr) {
    for (const auto &col : row)
      std::cout << col << " ";
    std::cout << std::endl;
  }
}

void append_csv_head(const std::string &file_name, const std::string &head);

double compute_ub_sigma(const std::vector<double> &ubs, double avg_ub);

#endif // COMMON_H
