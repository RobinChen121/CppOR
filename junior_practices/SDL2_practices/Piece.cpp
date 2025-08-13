/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 12/08/2025, 13:42
 * Description: 
 * 
 */

#include "Piece.h"

// 每次顺时针转 90 度
void Piece::rotate(const int times) {
  // std::vector<std::vector<int>> shape = shape;  // 复制一份
  for (int t = 0; t < times; t++) {
    // 1. 先垂直翻转（倒序行）
    std::reverse(shape.begin(), shape.end());

    // 2. 转置矩阵
    const int rows = static_cast<int>(shape.size());
    const int cols = static_cast<int>(shape[0].size());
    std::vector temp(cols, std::vector<int>(rows));
    for (int i = 0; i < rows; ++i)
      for (int j = 0; j < cols; ++j)
        temp[j][i] = shape[i][j];
    shape = std::move(temp);
  }
}
auto Piece::rotateRandom() -> void {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 2);
  const int random_time = dis(gen);
  rotate(random_time);
}
