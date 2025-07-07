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
#include <fstream>
#include <iomanip> // for std::fixed and std::setprecision
#include <iostream>
#include <type_traits> // for std::is_floating_point
#include <vector>

using Matrix = std::vector<std::vector<double>>;

Matrix removeDuplicateRows(const Matrix &mat);

// 自定义哈希函数
struct VectorHash {
  size_t operator()(const std::vector<double> &v) const {
    std::size_t seed = 0;
    for (const double num : v) {
      boost::hash_combine(seed, v);
    }
    return seed;
  }
};

// C++ 的模板必须在编译时已知它的定义，因此模板函数的实现一般会放在头文件（.h 或
// .hpp）中，而不是源文件（.cpp）
template <typename T1, typename T2>
std::vector<std::pair<T1, T2>> product(const std::vector<T1> &a, const std::vector<T2> &b) {
  std::vector<std::pair<T1, T2>> result;

  for (const auto &i : a) {
    for (const auto &j : b) {
      result.emplace_back(i, j);
    }
  }
  return result;
}

std::string toCSVLine(const std::vector<std::string> &row);

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

// 通用写入函数：支持任意类型的 vector
template <typename T>
void appendRowToCSV(const std::string &filename, const std::vector<T> &rowData) {
  std::ofstream file(filename, std::ios::app);
  // std::cerr 是 C++ 标准库里用于输出错误信息的一个输出流，用于输出错误、警告、调试信息等
  if (!file.is_open()) {
    std::cerr << "unable to open the file!" << filename << std::endl;
    return;
  }

  // 转换为字符串列表
  std::vector<std::string> stringRow;
  for (const auto &item : rowData) {
    stringRow.push_back(toString(item));
  }

  // 写入
  file << toCSVLine(stringRow) << "\n";
  file.close();
}

void appendHeadToCSV(const std::string &file_name, const std::string &head);

#endif // COMMON_H
