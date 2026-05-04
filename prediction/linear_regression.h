/*
 * Created by Zhen Chen on 2026/5/4.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef NEWSVENDOR_LINEAR_REGRESSION_H
#define NEWSVENDOR_LINEAR_REGRESSION_H

#include <vector>

class LinearRegression {
  std::vector<double> vector_X;
  std::vector<double> vector_y;

  int matrix_status{}; // 0 正常, 1 奇异, 2 维度不匹配

public:
  LinearRegression() {};
  LinearRegression(const std::vector<double> &vector_X, const std::vector<double> &vector_y)
      : vector_X(vector_X), vector_y(vector_y) {};

  std::vector<double> invert(const std::vector<double> &A, int col_num);
  std::vector<double> linearRegression(int n_features);
};

#endif // NEWSVENDOR_LINEAR_REGRESSION_H
