/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 14/10/2025, 20:39
 * Description: single input, 1 hidden layer, single output; no activation between hidden layer and
 * output.
 *
 */
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>
using namespace std;

// Sigmoid 激活函数
double sigmoid(double x) { return 1.0 / (1.0 + exp(-x)); }

// Sigmoid 导数
double sigmoid_derivative(double x) {
  double s = sigmoid(x);
  return s * (1 - s);
}

// 初始化随机数
double rand_weight() {
  // rand() 返回一个整数，范围 [0, RAND_MAX]
  return ((double)rand() / RAND_MAX) * 2 - 1; // [-1, 1]
}

int main() {
  srand((unsigned)time(0)); // 初始化随机数种, time(0) 获取当前时间

  // 一些超参数
  const int n_hidden = 10;
  const double lr = 0.1; // 学习率
  const int epochs = 2000;

  // 网络参数
  vector<double> w1(n_hidden); // 输入 -> 隐藏层 weights
  vector<double> b1(n_hidden); // 隐藏层 bias
  vector<double> w2(n_hidden); // 隐藏层 -> 输出层 weights
  double b2;                   // 输出层 bias

  // 初始化参数
  for (int i = 0; i < n_hidden; ++i) {
    w1[i] = rand_weight();
    b1[i] = rand_weight();
    w2[i] = rand_weight();
  }
  b2 = rand_weight();

  // 训练数据：y = 2x + 1
  vector<double> x_train, y_train;
  for (int i = 0; i < 50; ++i) {
    double x = -2.0 + 4.0 * i / 49.0;
    x_train.push_back(x);
    y_train.push_back(2 * x + 1);
  }

  // 训练循环
  for (int epoch = 0; epoch < epochs; ++epoch) {
    double total_loss = 0.0;

    for (int i = 0; i < x_train.size(); ++i) {
      double x = x_train[i];
      double y = y_train[i];

      // ====== 前向传播 ======
      vector<double> z1(n_hidden), o(n_hidden);
      for (int j = 0; j < n_hidden; ++j) {
        z1[j] = w1[j] * x + b1[j];
        o[j] = sigmoid(z1[j]);
      }

      double z2 = 0;
      for (int j = 0; j < n_hidden; ++j)
        z2 += w2[j] * o[j]; // 有一个累加
      z2 += b2;
      double y_pred = z2;

      // ====== 损失 (MSE) ======
      double loss = pow(y_pred - y, 2); // 这个损失也可以为 1/2 * 这个式子
      total_loss += loss;

      // ====== 反向传播 ======
      double dL_dy = 2 * (y_pred - y);

      // 输出层梯度
      vector<double> dL_dw2(n_hidden);
      for (int j = 0; j < n_hidden; ++j)
        dL_dw2[j] = dL_dy * o[j];
      double dL_db2 = dL_dy;

      // 隐藏层梯度
      vector<double> dL_dw1(n_hidden);
      vector<double> dL_db1(n_hidden);
      for (int j = 0; j < n_hidden; ++j) {
        double d_o = dL_dy * w2[j];
        double d_z1 = d_o * sigmoid_derivative(z1[j]);
        dL_dw1[j] = d_z1 * x;
        dL_db1[j] = d_z1;
      }

      // ====== 参数更新 ======
      for (int j = 0; j < n_hidden; ++j) {
        w1[j] -= lr * dL_dw1[j];
        b1[j] -= lr * dL_db1[j];
        w2[j] -= lr * dL_dw2[j];
      }
      b2 -= lr * dL_db2;
    }

    if (epoch % 200 == 0)
      cout << "Epoch " << epoch << "  Loss = " << total_loss / x_train.size() << endl;
  }

  // 预测测试
  cout << "\n=== Prediction ===" << endl;
  for (double x = -2; x <= 2; x += 1.0) {
    vector<double> z1(n_hidden), o(n_hidden);
    for (int j = 0; j < n_hidden; ++j) {
      z1[j] = w1[j] * x + b1[j];
      o[j] = sigmoid(z1[j]);
    }
    double z2 = 0;
    for (int j = 0; j < n_hidden; ++j)
      z2 += w2[j] * o[j];
    z2 += b2;
    cout << "x=" << x << "  y_pred=" << z2 << endl;
  }

  return 0;
}
