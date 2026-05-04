/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 11/04/2026, 11:18
 * Description:
 *
 */

#include "linear_regression.h"
#include <iostream>
#include <vector>

// declare first
std::vector<double> singleSmooth(const std::vector<double> &data, double alpha, int n_forecast);
std::vector<double> doubleSmooth(const std::vector<double> &data, double alpha, double beta,
                                 int n_forecast);
std::vector<double> TripleSmooth(const std::vector<double> &series, int season_len, double alpha,
                                 double beta, double gamma, int n_forecast);

#include <emscripten/bind.h>
// wasm_predict is just a unique identifier for the binding block,
// it is useless
EMSCRIPTEN_BINDINGS(wasm_predict) {
  // 注册 vector 类型，相当于在js中重新定义了几个数据类型
  emscripten::register_vector<double>("Vector");

  // 注册函数
  emscripten::function("singleSmooth", &singleSmooth);
  emscripten::function("doubleSmooth", &doubleSmooth);

  emscripten::class_<LinearRegression>("Regression") // 引号里是 js 调用时用的名称
      .constructor<std::vector<double>,
                   std::vector<double>>() // 这个是类的构造函数，() 非常关键，它代表了
                                          // “执行注册操作” 注册类里面的函数 前面的字符串名字是 js
      // 里面使用的名字
      .function("regression", &LinearRegression::linearRegression);
}

// 注册结构体
// emscripten::value_object<RegressionResult>("RegressionResult")
//     .field("weights", &RegressionResult::weights)
//     .field("status", &RegressionResult::status);

// 注册全局变量，不推荐
// 作为只读常量 constant (JS 不能修改它)，若要修改需要注册为 property
// emscripten::constant("matrixStatus", matrixStatus);

/**************************************/
// linear regression by matrix
// 计算矩阵转置

typedef std::vector<std::vector<double>> Matrix;
typedef std::vector<double> Vector;

Matrix transpose(const Matrix &A) {
  const size_t rows = A.size();
  const size_t cols = A[0].size();
  Matrix res(cols, Vector(rows));
  for (int i = 0; i < rows; ++i)
    for (int j = 0; j < cols; ++j)
      res[j][i] = A[i][j];
  return res;
}

// 矩阵乘法
Matrix multiply(const Matrix &A, const Matrix &B) {
  const size_t r1 = A.size();
  const size_t c1 = A[0].size();
  const size_t c2 = B[0].size();
  Matrix res(r1, Vector(c2, 0));
  for (int i = 0; i < r1; ++i)
    for (int k = 0; k < c1; ++k)
      for (int j = 0; j < c2; ++j)
        res[i][j] += A[i][k] * B[k][j];
  return res;
}

// 矩阵与向量乘法
Vector multiplyVec(const Matrix &A, const Vector &v) {
  Vector res(A.size(), 0);
  for (size_t i = 0; i < A.size(); ++i)
    for (size_t j = 0; j < v.size(); ++j)
      res[i] += A[i][j] * v[j];
  return res;
}

// 高斯-约当消元法求逆矩阵
Matrix invert2(const Matrix &A) {
  const size_t n = A.size();
  Matrix res(n, Vector(n, 0));
  Matrix temp = A;
  for (int i = 0; i < n; ++i)
    res[i][i] = 1.0;

  for (int i = 0; i < n; ++i) {
    // 寻找主元, a max value in each column
    int pivot = i;
    for (int j = i + 1; j < n; ++j)
      if (std::abs(temp[j][i]) > std::abs(temp[pivot][i]))
        pivot = j;
    // 交换值
    std::swap(temp[i], temp[pivot]);
    std::swap(res[i], res[pivot]);

    const double factor = temp[i][i];
    for (int j = 0; j < n; ++j) {
      temp[i][j] /= factor;
      res[i][j] /= factor;
    }

    for (int k = 0; k < n; ++k) {
      if (k != i) {
        const double f = temp[k][i];
        for (int j = 0; j < n; ++j) {
          temp[k][j] -= f * temp[i][j];
          res[k][j] -= f * res[i][j];
        }
      }
    }
  }
  return res;
}

Vector linearRegression2(const Matrix &X_raw, const Vector &y) {
  const size_t n = X_raw.size();
  const size_t features = X_raw[0].size();

  // 构造增广矩阵 X (添加一列 1 作为截距项)
  Matrix X(n, Vector(features + 1));
  for (int i = 0; i < n; ++i) {
    X[i][0] = 1.0;
    for (int j = 0; j < features; ++j)
      X[i][j + 1] = X_raw[i][j];
  }

  // 正规方程: w = (XT * X)^-1 * XT * y
  const Matrix XT = transpose(X);
  const Matrix XTX = multiply(XT, X);
  const Matrix XTX_inv = invert2(XTX);
  const Vector XTy = multiplyVec(XT, y);

  return multiplyVec(XTX_inv, XTy);
}

/*************************************/
// linear regression by vector

// 使用一维数组模拟矩阵索引: row_index * col_num + col_index
int getIndex(const int row_index, const int col_index, const int col_num) {
  return row_index * col_num + col_index;
}

// 高斯-约当消元法（含主元选择），针对连续内存优化
// res 是 A 的逆矩阵
std::vector<double> LinearRegression::invert(const std::vector<double> &A, const int col_num) {
  std::vector<double> temp = A;
  std::vector<double> res(col_num * col_num, 0.0);
  for (int i = 0; i < col_num; ++i)
    res[getIndex(i, i, col_num)] = 1.0;

  for (int i = 0; i < col_num; ++i) {
    // 寻找绝对值最大的主元，增强数值稳定性
    int pivot = i;
    for (int j = i + 1; j < col_num; ++j) {
      if (std::abs(temp[getIndex(j, i, col_num)]) > std::abs(temp[getIndex(pivot, i, col_num)])) {
        pivot = j;
      }
    }

    // 检查矩阵是否奇异
    if (std::abs(temp[getIndex(pivot, i, col_num)]) < 1e-12) {
      matrix_status = 1;
      return {};
    }

    // 交换行
    if (pivot != i) {
      for (int k = 0; k < col_num; ++k) {
        std::swap(temp[getIndex(i, k, col_num)], temp[getIndex(pivot, k, col_num)]);
        std::swap(res[getIndex(i, k, col_num)], res[getIndex(pivot, k, col_num)]);
      }
    }

    // 行变换
    const double factor = temp[getIndex(i, i, col_num)];
    for (int j = 0; j < col_num; ++j) {
      temp[getIndex(i, j, col_num)] /= factor;
      res[getIndex(i, j, col_num)] /= factor;
    }

    for (int k = 0; k < col_num; ++k) {
      if (k != i) {
        const double f = temp[getIndex(k, i, col_num)];
        for (int j = 0; j < col_num; ++j) {
          temp[getIndex(k, j, col_num)] -= f * temp[getIndex(i, j, col_num)];
          res[getIndex(k, j, col_num)] -= f * res[getIndex(i, j, col_num)];
        }
      }
    }
  }
  return res;
}

// 训练函数：输入一维化的 X 和 Y
std::vector<double> LinearRegression::linearRegression(const int n_features) {
  const size_t n_samples = vector_y.size();
  if (static_cast<int>(vector_X.size()) != n_samples * n_features || vector_y.size() != n_samples) {
    matrix_status = 2;
    return {};
  }

  const int dim = n_features + 1; // 包含截距项

  // 构造增广矩阵 X (Row-major)
  std::vector<double> X(n_samples * dim);
  for (int i = 0; i < n_samples; ++i) {
    X[getIndex(i, 0, dim)] = 1.0; // 截距项
    for (int j = 0; j < n_features; ++j) {
      X[getIndex(i, j + 1, dim)] = vector_X[getIndex(i, j, n_features)];
    }
  }

  // 计算 XT * X (结果大小为 dim * dim)
  std::vector<double> XTX(dim * dim, 0.0);
  for (int i = 0; i < dim; ++i) {
    for (int j = 0; j < dim; ++j) {
      for (int k = 0; k < n_samples; ++k) {
        XTX[getIndex(i, j, dim)] += X[getIndex(k, i, dim)] * X[getIndex(k, j, dim)];
      }
    }
  }

  // 计算 (XT * X)^-1
  const std::vector<double> XTX_inv = invert(XTX, dim);

  // 计算 XT * y
  std::vector<double> XTy(dim, 0.0);
  for (int i = 0; i < dim; ++i) {
    for (int k = 0; k < n_samples; ++k) {
      XTy[i] += X[getIndex(k, i, dim)] * vector_y[k];
    }
  }

  // 计算 w = XTX_inv * XTy
  std::vector<double> weights(dim, 0.0);
  for (int i = 0; i < dim; ++i) {
    for (int j = 0; j < dim; ++j) {
      weights[i] += XTX_inv[getIndex(i, j, dim)] * XTy[j];
    }
  }

  return weights;
}

std::vector<double> toVector(const std::vector<std::vector<double>> &matrix) {
  const size_t m = matrix.size();
  const size_t n = matrix[0].size();
  std::vector<double> res(m * n);
  for (size_t i = 0; i < m; ++i) {
    for (size_t j = 0; j < n; ++j) {
      res[i * n + j] = matrix[i][j];
    }
  }
  return res;
}

/*************************************/

std::vector<double> singleSmooth(const std::vector<double> &data, const double alpha,
                                 const int n_forecast) {
  const size_t n = data.size();
  std::vector<double> smooth(n + n_forecast);

  // 初始化：常见做法是 S0 = y0
  smooth[0] = data[0];

  // 平滑过程
  for (int t = 1; t <= n; ++t) {
    smooth[t] = alpha * data[t - 1] + (1 - alpha) * smooth[t - 1];
  }

  for (size_t t = n + 1; t < n + n_forecast; ++t) {
    smooth[t] = alpha * data[n - 1] + (1 - alpha) * smooth[t - 1];
  }

  return smooth;
}

std::vector<double> doubleSmooth(const std::vector<double> &data, const double alpha,
                                 const double beta, const int n_forecast) {
  const size_t n = data.size();
  if (n < 2)
    return data;

  std::vector<double> smooth(n + n_forecast);

  // 初始化
  // 初始水平 S0 为第一个点
  double level = data[0];
  // 初始趋势 b0 为前两个点的差值
  double trend = data[1] - data[0];

  smooth[0] = level; // 第一个点通常作为初始值
  smooth[1] = level + trend;

  for (int i = 2; i < n + 1; ++i) {
    const double last_level = level;

    // 更新水平 S_t
    level = alpha * data[i - 1] + (1 - alpha) * (level + trend);

    // 更新趋势 b_t
    trend = beta * (level - last_level) + (1 - beta) * trend;

    // 当前时刻的平滑值 (预测值)
    smooth[i] = level + trend;
  }

  for (size_t t = n + 1; t < n + n_forecast; ++t) {
    smooth[t] = level + static_cast<double>(t - n + 1) * trend;
  }

  return smooth;
}

std::vector<double> tripleSmooth(const std::vector<double> &series, const int season_len,
                                 const double alpha, const double beta, const double gamma,
                                 const int n_forecast) {

  const size_t n = series.size();
  std::vector<double> forecast(n + n_forecast);

  if (n < 2 * n_forecast)
    return series;

  // Initialize Seasonality
  std::vector<double> season_factor(season_len, 0.0);
  double sum1 = 0.0, sum2 = 0.0;
  for (int i = 0; i < season_len; ++i) {
    sum1 += series[i];
    sum2 += series[i + season_len];
    forecast[i] = series[i];
    forecast[i + season_len] = series[i + season_len];
  }
  const double avg1 = sum1 / season_len;
  const double avg2 = sum2 / season_len;

  for (int i = 0; i < season_len; ++i) {
    season_factor[i] = (series[i] / avg1 + series[i + season_len] / avg2) / 2;
  }

  // 2. Initialize Level and Trend
  double level = series[2 * season_len - 1] / season_factor[season_len - 1];
  double trend_sum = 0.0;
  for (int i = 0; i < season_len; ++i) {
    trend_sum += (series[season_len + i] - series[i]) / season_len;
  }
  double trend = trend_sum / season_len;

  // Triple Smoothing Loop
  forecast[2 * season_len] = (level + trend) * season_factor[0];
  for (int i = 2 * season_len + 1; i < n + 1; ++i) {
    const double last_level = level;

    // Update Level
    level = alpha * (series[i - 1] / season_factor[(i - 1) % season_len]) +
            (1 - alpha) * (level + trend);
    // Update Trend
    trend = beta * (level - last_level) + (1 - beta) * trend;
    // Update Seasonality
    season_factor[(i - 1) % season_len] =
        gamma * (series[i - 1] / level) + (1 - gamma) * season_factor[(i - 1) % season_len];

    forecast[i] = (level + trend) * season_factor[i % season_len];
  }

  // 4. Generate Future Forecast
  for (size_t i = n + 1; i < n + n_forecast; ++i) {
    forecast[i] = (level + trend) * season_factor[i % season_len];
  }

  return forecast;
}

// int main() {
//   const std::vector<double> data = {10.21, 23.01, 10.97, 14.59, 29.44, 16.8,
//                                     18.86, 38.9,  18.61, 24.2,  48.9,  22.78};
//
//   int n_forecast = 5;
//   auto result = singleSmooth(data, 0.3, n_forecast);
//   for (const double v : result) {
//     std::cout << v << " ";
//   }
//   std::cout << std::endl;
//
//   std::cout << "**********************" << std::endl;
//   result = doubleSmooth(data, 0.2, 0.2, n_forecast);
//   for (const double v : result) {
//     std::cout << v << " ";
//   }
//   std::cout << std::endl;
//
//   std::cout << "**********************" << std::endl;
//   result = tripleSmooth(data, 3, 0.2, 0.2, 0.2, n_forecast);
//   for (const double v : result) {
//     std::cout << v << " ";
//   }
//   std::cout << std::endl;
//
//   std::cout << "**********************" << std::endl;
//   // 示例数据: y = 5 + 2*x1 + 0.5*x2
//   const Matrix X = {{1, 2}, {2, 1}, {4, 3}, {3, 5}};
//   const Vector y = {8, 9.5, 14.5, 13.5};
//   const Vector weights2 = linearRegression2(X, y);
//
//   std::cout << "Intercept: " << weights2[0] << "\nWeights: ";
//   for (size_t i = 1; i < weights2.size(); ++i)
//     std::cout << weights2[i] << " ";
//   std::cout << std::endl;
//
//   std::cout << "**********************" << std::endl;
//   // 示例数据: y = 5 + 2*x1 + 0.5*x2
//   const int n_features = static_cast<int>(X[0].size());
//   LinearRegression model(toVector(X), y);
//   const Vector weights = model.linearRegression(n_features);
//
//   std::cout << "Intercept: " << weights[0] << "\nWeights: ";
//   for (size_t i = 1; i < weights.size(); ++i)
//     std::cout << weights[i] << " ";
//   std::cout << std::endl;
//
//   return 0;
// }