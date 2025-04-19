/*
 * Created by Zhen Chen on 2025/3/13.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "common.h"

#include <iostream>
#include <unordered_set>
#include <vector>

#define VAR_NAME(x) #x

Matrix removeDuplicateRows(const Matrix &mat) {
  std::unordered_set<std::vector<double>, VectorHash> uniqueRows(mat.begin(), mat.end());
  // 用哈希表去重
  return {uniqueRows.begin(), uniqueRows.end()};
}

// 卸入 csv 一行字符串的代码为 file << "4,Diana,88.7\n";
// 格式化一行为 CSV（加引号、转义等）
std::string toCSVLine(const std::vector<std::string> &row) {
  std::string line;
  for (size_t i = 0; i < row.size(); ++i) {
    std::string cell = row[i];

    // 如果字段中有引号，使用双引号包裹并转义
    // 当你在字符串里找某个子串、字符，没找到的时候，就返回 std::string::npos
    // find() find the position of the first character of the found substring or npos
    // if no such substring is found.
    if (cell.find(',') != std::string::npos || cell.find('"') != std::string::npos) {
      size_t pos = 0;
      // \" 是 转义字符，在 C++ 字符串中它表示一个真正的双引号字符
      while ((pos = cell.find('"', pos)) != std::string::npos) {
        cell.insert(pos, "\"");
        pos += 2;
      }
      cell = "\"" + cell + "\"";
    }

    line += cell;
    if (i != row.size() - 1)
      line += ",";
  }
  return line;
}

void appendHeadToCSV(const std::string &file_name, const std::string &head) {
  std::ofstream file(file_name, std::ios::app);
  file << head;
  file.close();
}

// int main() {
//   appendRowToCSV("output.csv", std::vector<int>{7, 8, 9});
//   appendRowToCSV("output.csv", std::vector<double>{1.23, 4.56});
//   appendRowToCSV("output.csv", std::vector<std::string>{"Name", "With,Comma", "Quote\"Inside"});
//
//   double r1 = 0.1;
//   double r2 = 1;
//   appendRowToCSV("output.csv", std::vector<std::string>{VAR_NAME(r1), VAR_NAME(r2)});
//
//
//     return 0;
//   }

// const Matrix mat = {
//     {1, 2, 3},
//     {4, 5, 6},
//     {1, 2, 3}, // 重复行
//     {7, 8, 9},
//     {4, 5, 6} // 重复行
// };
//
// const Matrix uniqueMat = removeDuplicateRows(mat);
//
// std::unordered_set<std::vector<double>, VectorHash> sets;
// std::vector<double> first_row = {10, 12, 43};
// std::vector<double> second_row(3, 20);
// std::vector<double> third_row(3, 20);
// sets.insert(first_row);
// sets.insert(second_row);
//
// std::cout << "Matrix after removing duplicate rows:\n";
// for (const auto &row : uniqueMat) {
//   for (const double val : row) {
//     std::cout << val << " ";
//   }
//   std::cout << std::endl;
// }
//
// std::cout << "sets:\n";
// for (const auto &row : sets) {
//   for (const double val : row) {
//     std::cout << val << " ";
//   }
//   std::cout << std::endl;
// }
