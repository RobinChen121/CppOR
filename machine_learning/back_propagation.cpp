/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 18/10/2025, 16:59
 * Description: ReLu performs best, about 97% or 100% accuracy.
 * Softmax can be uses in the lat output, about same performance with ReLU.
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

struct Sample {
  std::vector<double> x;
  std::vector<double> y;
};

// double sigmoid(const double x) { return 1.0 / (1.0 + exp(-x)); }
// double dSigmoid(const double y) { return y * (1.0 - y); } // derivative wrt output

double randd() {
  static std::random_device rd; // 真随机种子
  static std::mt19937 gen(42);  // rd()                           // 梅森旋转算法引擎
  static std::uniform_real_distribution<double> dist(-1.0, 1.0); // 区间 [-1,1]
  return dist(gen);
}

std::vector<Sample> read_data(const std::string &file_name) {
  std::ifstream fin(file_name);
  if (!fin) {
    std::cerr << "Cannot open iris.csv\n";
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

int main() {
  // srand(42); // seed

  // put the file in the current cmake-build-debug folder
  // or put one lion of codes in the CMakeLists.txt file:
  // file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})
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
  std::mt19937 g(42); // rd(),  随机数生成器
  std::ranges::shuffle(data, g);
  int train_size = static_cast<int>(static_cast<double>(data.size()) * 0.8);
  std::vector train(data.begin(), data.begin() + train_size);
  std::vector test(data.begin() + train_size, data.end());

  // network: 4 -> 20 -> 10 -> 3
  int input_num = 4, hidden1_num = 20, hidden2_num = 10, output_num = 3;
  double lr = 0.1; // swish need low learning rate such as 0.001
  int epochs = 6000;
  auto activation_type = ActivationType::ReLU;

  std::vector w1(input_num, std::vector<double>(hidden1_num));
  std::vector w2(hidden1_num, std::vector<double>(hidden2_num));
  std::vector w3(hidden2_num, std::vector<double>(output_num));
  std::vector<double> b1(hidden1_num), b2(hidden2_num),
      b3(output_num); // initial values for b are 0
  for (auto &v : w1)
    for (auto &x : v)
      x = randd() * 0.5;
  for (auto &v : w2)
    for (auto &x : v)
      x = randd() * 0.5;
  for (auto &v : w3)
    for (auto &x : v)
      x = randd() * 0.5;

  // train
  for (int epoch = 0; epoch < epochs; epoch++) {
    double loss = 0;
    for (auto &[x, y] : train) {
      // forward pass
      std::vector<double> h1(hidden1_num), hidden1_output(hidden1_num);
      for (int j = 0; j < hidden1_num; j++) {
        double net = b1[j];
        for (int i = 0; i < input_num; i++)
          net += x[i] * w1[i][j];
        hidden1_output[j] = activate(net, activation_type);
      }

      std::vector<double> hidden2_output(hidden2_num);
      for (int j = 0; j < hidden2_num; j++) {
        double net = b2[j];
        for (int i = 0; i < hidden1_num; i++)
          net += hidden1_output[i] * w2[i][j];
        hidden2_output[j] = activate(net, activation_type);
      }

      std::vector<double> final_output(output_num);
      for (int k = 0; k < output_num; k++) {
        double net = b3[k];
        for (int j = 0; j < hidden2_num; j++)
          net += hidden2_output[j] * w3[j][k];
        final_output[k] = activate(net, activation_type);
        // final_output[k] = net;
      }
      // final_output = Softmax::activate(final_output);

      // compute deltas
      std::vector<double> delta_out(output_num);
      for (int k = 0; k < output_num; k++) {
        double err = y[k] - final_output[k];
        delta_out[k] = err * derivative(final_output[k], activation_type);
        loss += 0.5 * err * err;
      }

      std::vector<double> delta_h2(hidden2_num);
      for (int j = 0; j < hidden2_num; j++) {
        double sum = 0;
        for (int k = 0; k < output_num; k++)
          sum += delta_out[k] * w3[j][k];
        delta_h2[j] = sum * derivative(hidden2_output[j], activation_type);
      }

      std::vector<double> delta_h1(hidden1_num);
      for (int j = 0; j < hidden1_num; j++) {
        double sum = 0;
        for (int k = 0; k < hidden2_num; k++)
          sum += delta_h2[k] * w2[j][k];
        delta_h1[j] = sum * derivative(hidden1_output[j], activation_type);
      }

      // update w3, b3
      for (int j = 0; j < hidden2_num; j++)
        for (int k = 0; k < output_num; k++)
          w3[j][k] += lr * delta_out[k] * hidden2_output[j];
      for (int k = 0; k < output_num; k++)
        b3[k] += lr * delta_out[k];

      // update w2, b2
      for (int j = 0; j < hidden1_num; j++)
        for (int k = 0; k < hidden2_num; k++)
          w2[j][k] += lr * delta_h2[k] * hidden1_output[j];
      for (int k = 0; k < hidden2_num; k++)
        b2[k] += lr * delta_h2[k];

      // update w1, b1
      for (int i = 0; i < input_num; i++)
        for (int j = 0; j < hidden1_num; j++)
          w1[i][j] += lr * delta_h1[j] * x[i];
      for (int j = 0; j < hidden1_num; j++)
        b1[j] += lr * delta_h1[j];
    }

    if (epoch % 500 == 0)
      std::cout << "Epoch " << epoch << " loss=" << loss / static_cast<double>(train.size())
                << "\n";
  }

  // test
  int correct = 0;
  for (auto &[x, y] : test) {
    std::vector<double> h1(hidden1_num);
    for (int j = 0; j < hidden1_num; j++) {
      double net = b1[j];
      for (int i = 0; i < input_num; i++)
        net += x[i] * w1[i][j];
      h1[j] = activate(net, activation_type);
    }
    std::vector<double> h2(hidden2_num);
    for (int j = 0; j < hidden2_num; j++) {
      double net = b2[j];
      for (int i = 0; i < hidden1_num; i++)
        net += h1[i] * w2[i][j];
      h2[j] = activate(net, activation_type);
    }
    std::vector<double> output_test(output_num);
    for (int k = 0; k < output_num; k++) {
      double net = b3[k];
      for (int j = 0; j < hidden2_num; j++)
        net += h2[j] * w3[j][k];
      output_test[k] = activate(net, activation_type);
      // output_test[k] = net;
    }
    // output_test = Softmax::activate(output_test);

    // get the index of the max element
    auto pred = std::ranges::max_element(output_test) - output_test.begin();
    if (auto truth = std::ranges::max_element(y) - y.begin(); pred == truth)
      correct++;
  }

  std::cout << "Accuracy: " << static_cast<double>(correct) / static_cast<double>(test.size()) * 100
            << "%\n";
}
