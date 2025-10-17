/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 14/10/2025, 20:39
 * Description: single input, 0 hidden layer, single output; no activation between hidden layer and
 * output. This
 *
 */
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>
using namespace std;

// 初始化随机数
double rand_weight() {
  // rand() 返回一个整数，范围 [0, RAND_MAX]
  return ((double)rand() / RAND_MAX) * 2 - 1; // [-1, 1]
}

int main() {
  srand((unsigned)time(0)); // 初始化随机数种, time(0) 获取当前时间

  // 一些超参数
  const int n_hidden = 0;
  const double lr = 0.1; // 学习率
  const int epochs = 2000;

  // 初始化参数
  double w = rand_weight();
  double b = rand_weight();

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
      const double y = y_train[i];

      // ====== 前向传播 ======
      const double y_pred = w * x + b;

      // ====== 损失 (MSE) ======
      const double loss = 0.5 * pow(y_pred - y, 2); // 这个损失也可以为 1/2 * 这个式子
      total_loss += loss;

      // ====== 反向传播 ======
      const double dL_dy = (y_pred - y);

      w -= lr * dL_dy * x;
      b -= lr * dL_dy;
    }

    if (epoch % 200 == 0)
      cout << "Epoch " << epoch << "  Loss = " << total_loss / x_train.size() << endl;
  }

  // 预测测试
  cout << "*************" << endl;
  cout << "w = " << w << ", b = " << b;
  cout << "\n=== Prediction ===" << endl;
  for (double x = -2; x <= 2; x += 1.0) {
    double z = w * x + b;
    cout << "x=" << x << "  y_pred=" << z << endl;
  }

  return 0;
}
