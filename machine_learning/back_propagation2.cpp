/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 18/10/2025, 16:59
 * Description: test whether its performance can be as good and stable as pytorch;
 * Pytorch has more precision in the computation and guarantee more stability;
 *
 */

#include "activation.h"
#include <algorithm>
#include <filesystem> // for file operations
#include <fstream>    // for input, output stream
#include <iostream>
#include <random>
#include <sstream> // ← 加上这行
#include <vector>

std::mt19937 gen(42); // 全局固定种子
struct Sample {
  std::vector<double> x;
  std::vector<double> y;
};

// double sigmoid(const double x) { return 1.0 / (1.0 + exp(-x)); }
// double dSigmoid(const double y) { return y * (1.0 - y); } // derivative wrt output

double randd() {
  // static std::random_device rd;                                  // 真随机种子
  // static std::mt19937 gen(42);                                   // rd()  固定种子
  static std::uniform_real_distribution<double> dist(-1.0, 1.0); // 区间 [-1,1]
  return dist(gen);
}

std::vector<Sample> read_data(const std::string &file_name) {
  std::ifstream fin(file_name);
  if (!fin) {
    std::cerr << "Cannot open this file\n";
    std::exit(EXIT_FAILURE); // 直接终止程序, EXIT_FAILURE 的值为1
  }

  std::string line;
  getline(fin, line);
  std::vector<Sample> data;
  while (getline(fin, line)) {
    std::stringstream ss(line);
    std::string cell;
    std::vector<double> x(4);
    for (int i = 0; i < 4; i++) {
      std::getline(ss, cell, ',');
      x[i] = std::stod(cell); // string to double function: stod
    }
    getline(ss, cell, ','); // 分隔符 ',', 读到它就停止
    std::vector<double> y(3, 0.0);
    if (cell == "setosa")
      y[0] = 1;
    else if (cell == "versicolor")
      y[1] = 1;
    else
      y[2] = 1;
    data.push_back({x, y});
  }
  return data;
}

double rand_uniform(double a, double b) {
  // static std::random_device rd;
  // static std::mt19937 gen(42);
  std::uniform_real_distribution<double> dist(a, b);
  return dist(gen);
}

// Xavier 初始化, for sigmoid, tanh
void xavier_init(std::vector<std::vector<double>> &weights, int n_in, int n_out) {
  double limit = std::sqrt(6.0 / (n_in + n_out)); // [-limit, limit]
  for (auto &row : weights)
    for (auto &w : row)
      w = rand_uniform(-limit, limit);
}

// He 初始化, for ReLU
void he_init(std::vector<std::vector<double>> &weights, int n_in) {
  double stddev = std::sqrt(2.0 / n_in); // 标准差
  // static std::random_device rd;
  // static std::mt19937 gen(42);
  std::normal_distribution<double> dist(0.0, stddev);

  for (auto &row : weights)
    for (auto &w : row)
      w = dist(gen);
}

int main() {
  // srand(42); // seed

  std::string file_name = "iris.csv";
  auto data = read_data(file_name);

  // normalize
  for (int j = 0; j < 4; j++) {
    double mn = 1e9, mx = -1e9;
    for (auto &[x, y] : data) {
      mn = std::min(mn, x[j]);
      mx = std::max(mx, x[j]);
    }
    for (auto &[x, y] : data)
      x[j] = (x[j] - mn) / (mx - mn);
  }

  std::random_device rd;
  std::mt19937 g(rd());          // rd(),  随机数生成器
  std::ranges::shuffle(data, g); // 打乱数据
  int train_size = static_cast<int>(static_cast<double>(data.size()) * 0.8);
  std::vector<Sample> train(data.begin(), data.begin() + train_size);
  std::vector<Sample> test(data.begin() + train_size, data.end());

  // network: 4 -> 10 -> 3
  int input_num = 4, hidden_num = 10, output_num = 3;
  double lr = 0.1; // swish/softmax need low learning rate such as 0.001, sigmoid 0.1
  int epochs = 5000;
  int batch_size = 1;
  auto activation_type = ActivationType::Sigmoid;

  std::vector w1(input_num, std::vector<double>(hidden_num));
  std::vector w2(hidden_num, std::vector<double>(output_num));
  std::vector<double> b1(hidden_num), b2(output_num); // initial values for b are 0

  // 下面的权重初始化更稳定
  xavier_init(w1, input_num, hidden_num);
  xavier_init(w2, hidden_num, output_num);

  // 累积梯度，for batch
  std::vector<std::vector<double>> grad_w1(input_num, std::vector<double>(hidden_num, 0.0));
  std::vector<std::vector<double>> grad_w2(hidden_num, std::vector<double>(output_num, 0.0));
  std::vector<double> grad_b1(hidden_num, 0.0);
  std::vector<double> grad_b2(output_num, 0.0);

  // train
  for (int epoch = 0; epoch < epochs; epoch++) {
    double loss = 0;
    int sample_id = 0;
    for (auto &[x, y] : train) {
      // t++;
      sample_id++;
      // forward pass
      std::vector<double> hidden(hidden_num);
      for (int j = 0; j < hidden_num; j++) {
        double net = b1[j];
        for (int i = 0; i < input_num; i++)
          net += x[i] * w1[i][j];
        hidden[j] = activate(net, ActivationType::Sigmoid);
      }

      std::vector<double> final_output(output_num);
      for (int k = 0; k < output_num; k++) {
        double net = b2[k];
        for (int j = 0; j < hidden_num; j++)
          net += hidden[j] * w2[j][k];
        if (activation_type != ActivationType::Softmax)
          final_output[k] = activate(net, activation_type);
        else
          final_output[k] = net;
      }
      if (activation_type == ActivationType::Softmax)
        final_output = Softmax::activate(final_output);

      // compute deltas
      std::vector<double> delta_out(output_num);
      for (int k = 0; k < output_num; k++) {
        double err = final_output[k] - y[k];
        if (activation_type != ActivationType::Softmax) {
          delta_out[k] = err * derivative(final_output[k], activation_type);
          loss += err * err;
        } else {
          delta_out[k] = final_output[k] - y[k];
          loss += -y[k] * log(final_output[k] + 1e-12);
        }
      }

      std::vector<double> delta_h(hidden_num);
      for (int j = 0; j < hidden_num; j++) {
        double sum = 0;
        for (int k = 0; k < output_num; k++)
          sum += delta_out[k] * w2[j][k];
        delta_h[j] = sum * derivative(hidden[j], activation_type); //
      }

      // SGD update w2, b2
      for (int j = 0; j < hidden_num; j++)
        for (int k = 0; k < output_num; k++)
          w2[j][k] += -lr * delta_out[k] * hidden[j];
      for (int k = 0; k < output_num; k++)
        b2[k] += -lr * delta_out[k];

      // update w1, b1
      for (int i = 0; i < input_num; i++)
        for (int j = 0; j < hidden_num; j++)
          w1[i][j] += -lr * delta_h[j] * x[i];
      for (int j = 0; j < hidden_num; j++)
        b1[j] += -lr * delta_h[j];
    }

    if (epoch % 500 == 0)
      std::cout << "Epoch " << epoch << " loss=" << loss / static_cast<double>(train.size())
                << "\n";
  }

  // test
  int correct = 0;
  for (auto &[x, y] : test) {
    std::vector<double> h1(hidden_num);
    for (int j = 0; j < hidden_num; j++) {
      double net = b1[j];
      for (int i = 0; i < input_num; i++)
        net += x[i] * w1[i][j];
      h1[j] = activate(net, activation_type);
    }
    std::vector<double> output_test(output_num);
    for (int k = 0; k < output_num; k++) {
      double net = b2[k];
      for (int j = 0; j < hidden_num; j++)
        net += h1[j] * w2[j][k];
      if (activation_type != ActivationType::Softmax)
        output_test[k] = activate(net, activation_type);
      else
        output_test[k] = net;
    }
    if (activation_type == ActivationType::Softmax)
      output_test = Softmax::activate(output_test);

    // get the index of the max element
    auto pred = std::ranges::max_element(output_test) - output_test.begin();
    if (auto truth = std::ranges::max_element(y) - y.begin(); pred == truth)
      correct++;
  }

  std::cout << "Accuracy: " << static_cast<double>(correct) / static_cast<double>(test.size()) * 100
            << "%\n";
}

// // === 累积梯度, for batch ===
// for (int j = 0; j < hidden_num; j++)
//   for (int k = 0; k < output_num; k++)
//     grad_w2[j][k] += delta_out[k] * hidden_output[j];
// for (int k = 0; k < output_num; k++)
//   grad_b2[k] += delta_out[k];
//
// for (int i = 0; i < input_num; i++)
//   for (int j = 0; j < hidden_num; j++)
//     grad_w1[i][j] += delta_h[j] * x[i];
// for (int j = 0; j < hidden_num; j++)
//   grad_b1[j] += delta_h[j];
//
// // === batch 更新参数 ===
// if (sample_id % batch_size == 0) {
//   double scale = lr / batch_size;
//   // w2,b2
//   for (int j = 0; j < hidden_num; j++)
//     for (int k = 0; k < output_num; k++) {
//       w2[j][k] -= scale * grad_w2[j][k];
//       grad_w2[j][k] = 0;
//     }
//   for (int k = 0; k < output_num; k++) {
//     b2[k] -= scale * grad_b2[k];
//     grad_b2[k] = 0;
//   }
//
//   // 同理更新  w1,b1 (清零渐变)
//   for (int j = 0; j < input_num; j++)
//     for (int k = 0; k < hidden_num; k++) {
//       w1[j][k] -= scale * grad_w1[j][k];
//       grad_w1[j][k] = 0;
//     }
//   for (int k = 0; k < hidden_num; k++) {
//     b1[k] -= scale * grad_b1[k];
//     grad_b1[k] = 0;
//   }
// }