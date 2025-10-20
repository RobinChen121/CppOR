/*
 * Created by Zhen Chen on 2025/10/20.
 * Email: chen.zhen5526@gmail.com
 * Description: no training in this code example.
 *
 *
 */
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

using namespace std;

// 激活函数：tanh
double tanh_act(double x) {
  return tanh(x);
}

// RNN 单步更新
struct SimpleRNN {
  double W_x, W_h, W_y;
  double b_h, b_y;
  double h; // 隐藏状态

  SimpleRNN() {
    srand(time(0));
    // 随机初始化权重
    W_x = ((double)rand() / RAND_MAX - 0.5); // a random number between -0.5 ～ 0.5
    W_h = ((double)rand() / RAND_MAX - 0.5);
    W_y = ((double)rand() / RAND_MAX - 0.5);
    b_h = ((double)rand() / RAND_MAX - 0.5);
    b_y = ((double)rand() / RAND_MAX - 0.5);
    h = 0.0;
  }

  // 前向传播
  double forward(double x_t) {
    // 1. 更新隐藏状态
    h = tanh_act(W_x * x_t + W_h * h + b_h);
    // 2. 输出
    double y = W_y * h + b_y;
    return y;
  }
};

int main() {
  // 输入序列：简单线性趋势
  vector<double> inputs = {1, 2, 3, 4, 5};

  SimpleRNN rnn;

  cout << "=== RNN 前向传播 ===" << endl;
  double output = 0.0;
  for (size_t t = 0; t < inputs.size(); t++) {
    output = rnn.forward(inputs[t]);
    cout << "t = " << t + 1
         << " | x_t = " << inputs[t]
         << " | h_t = " << rnn.h
         << " | y_t = " << output << endl;
  }

  cout << "\n预测下一个值（t=6）..." << endl;
  double next_input = 6.0;
  double pred = rnn.forward(next_input);
  cout << "Predicted y_6 = " << pred << endl;

  return 0;
}
