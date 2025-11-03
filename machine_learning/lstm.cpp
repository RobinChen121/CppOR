/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 01/11/2025, 20:14
 * Description:
 *
 */
#include <Eigen/Dense>
#include <cmath>
#include <iostream>
#include <vector>

using namespace Eigen;

#include "../utils/matplotlibcpp.h"
namespace plt = matplotlibcpp;

void draw_pic(const std::vector<double> &prediction, const std::vector<double> &true_value) {
  // 强制使用 TkAgg 后端，避免 Qt6Agg 卡死
  plt::backend("TkAgg"); // 等价于 python 中的命令 matplotlib.use("TkAgg")
  std::vector<double> x(prediction.size());
  for (int i = 0; i < prediction.size(); ++i) {
    x[i] = static_cast<double>(i);
  }

  plt::plot(x, true_value, {{"label", "true value"}});
  plt::plot(x, prediction, {{"label", "prediction value"}});
  plt::grid(true);
  plt::legend(); // 显示图例
  plt::show();
}

// Sigmoid
MatrixXd sigmoid(const MatrixXd &x) { return 1.0 / (1.0 + (-x.array()).exp()); }
MatrixXd sigmoid_derivative(const MatrixXd &x) { return x.array() * (1.0 - x.array()); }
MatrixXd tanh_derivative(const MatrixXd &x) { return 1.0 - x.array().tanh().square(); }

class LSTM {
public:
  int input_size, hidden_size;
  double lr;
  double beta1 = 0.9, beta2 = 0.999, epsilon = 1e-8;
  int t_adam = 0;

  // LSTM 参数
  MatrixXd W_i, W_f, W_o, W_g;
  MatrixXd U_i, U_f, U_o, U_g;
  VectorXd b_i, b_f, b_o, b_g;

  // Adam 一阶、二阶矩
  MatrixXd m_W_i, m_W_f, m_W_o, m_W_g;
  MatrixXd m_U_i, m_U_f, m_U_o, m_U_g;
  VectorXd m_b_i, m_b_f, m_b_o, m_b_g;
  MatrixXd v_W_i, v_W_f, v_W_o, v_W_g;
  MatrixXd v_U_i, v_U_f, v_U_o, v_U_g;
  VectorXd v_b_i, v_b_f, v_b_o, v_b_g;

  // 用于存储前向传播中各变量数据
  std::vector<MatrixXd> h;
  std::vector<MatrixXd> c;
  std::vector<MatrixXd> i;
  std::vector<MatrixXd> f;
  std::vector<MatrixXd> o;
  std::vector<MatrixXd> g;

  LSTM(const int input_size_, const int hidden_size_, const double lr_ = 0.001)
      : input_size(input_size_), hidden_size(hidden_size_), lr(lr_) {

    W_i = MatrixXd::Random(hidden_size, input_size) * 0.1;
    W_f = MatrixXd::Random(hidden_size, input_size) * 0.1;
    W_o = MatrixXd::Random(hidden_size, input_size) * 0.1;
    W_g = MatrixXd::Random(hidden_size, input_size) * 0.1;

    U_i = MatrixXd::Random(hidden_size, hidden_size) * 0.1;
    U_f = MatrixXd::Random(hidden_size, hidden_size) * 0.1;
    U_o = MatrixXd::Random(hidden_size, hidden_size) * 0.1;
    U_g = MatrixXd::Random(hidden_size, hidden_size) * 0.1;

    b_i = VectorXd::Zero(hidden_size);
    b_f = VectorXd::Zero(hidden_size);
    b_o = VectorXd::Zero(hidden_size);
    b_g = VectorXd::Zero(hidden_size);

    // Adam
    m_W_i = MatrixXd::Zero(hidden_size, input_size);
    m_W_f = m_W_i;
    m_W_o = m_W_i;
    m_W_g = m_W_i;
    m_U_i = MatrixXd::Zero(hidden_size, hidden_size);
    m_U_f = m_U_i;
    m_U_o = m_U_i;
    m_U_g = m_U_i;
    m_b_i = VectorXd::Zero(hidden_size);
    m_b_f = m_b_i;
    m_b_o = m_b_i;
    m_b_g = m_b_i;
    v_W_i = m_W_i;
    v_W_f = m_W_i;
    v_W_o = m_W_i;
    v_W_g = m_W_i;
    v_U_i = m_U_i;
    v_U_f = m_U_i;
    v_U_o = m_U_i;
    v_U_g = m_U_i;
    v_b_i = m_b_i;
    v_b_f = m_b_i;
    v_b_o = m_b_i;
    v_b_g = m_b_i;
  }

  void forward(const MatrixXd &x) {
    const size_t T = x.rows(); // x 的每一行是一个时间步
    if (h.size() != T) {
      h.resize(T);
      c.resize(T);
      i.resize(T);
      f.resize(T);
      o.resize(T);
      g.resize(T);
    }
    for (size_t t = 0; t < T; t++) {
      h[t] = MatrixXd::Zero(hidden_size, 1);
      c[t] = MatrixXd::Zero(hidden_size, 1);
      i[t] = MatrixXd::Zero(hidden_size, 1);
      f[t] = MatrixXd::Zero(hidden_size, 1);
      o[t] = MatrixXd::Zero(hidden_size, 1);
      g[t] = MatrixXd::Zero(hidden_size, 1);
    }

    // std::cout << std::endl; // 换行并刷新
    auto last_c = c[0];
    auto last_h = h[0];
    for (size_t t = 0; t < T; ++t) {
      auto x_t = x.row(static_cast<Index>(t)).transpose();
      i[t] = sigmoid(W_i * x_t + U_i * last_h + b_i);
      f[t] = sigmoid(W_f * x_t + U_f * last_h + b_f);
      o[t] = sigmoid(W_o * x_t + U_o * last_h + b_o);
      // .array() 的作用是将一个矩阵或向量 从线性代数（matrix）对象转换为数组（Array）对象，
      // 从而可以进行 逐元素（element-wise）操作
      // .tanh() 是 Eigen 数组对象自带的逐元素双曲正切函数
      g[t] = (W_g * x_t + U_g * last_h + b_g).array().tanh();
      c[t] = i[t].array() * g[t].array() + f[t].array() * last_c.array();
      h[t] = o[t].array() * c[t].array().tanh();

      last_c = c[t];
      last_h = h[t];
    }
  }

  void backward_with_dh(const MatrixXd &x, const MatrixXd &dh_external) {
    const size_t T = x.rows();
    MatrixXd dW_i = MatrixXd::Zero(hidden_size, input_size);
    MatrixXd dW_f = MatrixXd::Zero(hidden_size, input_size);
    MatrixXd dW_o = MatrixXd::Zero(hidden_size, input_size);
    MatrixXd dW_g = MatrixXd::Zero(hidden_size, input_size);

    MatrixXd dU_i = MatrixXd::Zero(hidden_size, hidden_size);
    MatrixXd dU_f = MatrixXd::Zero(hidden_size, hidden_size);
    MatrixXd dU_o = MatrixXd::Zero(hidden_size, hidden_size);
    MatrixXd dU_g = MatrixXd::Zero(hidden_size, hidden_size);

    VectorXd db_i = VectorXd::Zero(hidden_size);
    VectorXd db_f = VectorXd::Zero(hidden_size);
    VectorXd db_o = VectorXd::Zero(hidden_size);
    VectorXd db_g = VectorXd::Zero(hidden_size);

    MatrixXd delta_h = MatrixXd::Zero(hidden_size, 1);
    MatrixXd delta_c = MatrixXd::Zero(hidden_size, 1);
    MatrixXd delta_f = MatrixXd::Zero(hidden_size, 1);
    MatrixXd delta_g = MatrixXd::Zero(hidden_size, 1);
    MatrixXd delta_i = MatrixXd::Zero(hidden_size, 1);
    MatrixXd delta_o = MatrixXd::Zero(hidden_size, 1);
    for (int t = static_cast<int>(T - 1); t >= 0; --t) {
      if (t == T - 1) {
        delta_h = dh_external;
      } else {
        delta_h = U_f.transpose() * delta_f + U_g.transpose() * delta_g +
                  U_o.transpose() * delta_o + U_i.transpose() * delta_i;
      }
      if (t == T - 1)
        delta_c = delta_h.array() * o[t].array() * (1 - c[t].array().tanh().square());
      else {
        delta_c = delta_h.array() * o[t].array() * (1 - c[t].array().tanh().square()) +
                  delta_c.array() * f[t + 1].array();
      }

      delta_i = delta_c.array() * g[t].array() * i[t].array() * (1 - i[t].array());
      delta_f = delta_c.array() * c[t].array() * f[t].array() * (1 - f[t].array());
      delta_o = delta_h.array() * c[t].array().tanh() * o[t].array() * (1 - o[t].array());
      delta_g = delta_c.array() * i[t].array() * (1 - g[t].array().square());

      dW_i += delta_i * x.row(t).transpose(); // 这个 x[t] 不需要转置，因为x[t]是一个行向量
      if (t > 0)
        dU_i += delta_i * h[t - 1].transpose();
      db_i += delta_i.col(0);
      dW_f += delta_f * x.row(t).transpose();
      if (t > 0)
        dU_f += delta_f * h[t - 1].transpose();
      db_f += delta_f.col(0);
      dW_o += delta_o * x.row(t).transpose();
      if (t > 0)
        dU_o += delta_o * h[t - 1].transpose();
      db_o += delta_o.col(0);
      dW_g += delta_g * x.row(t).transpose();
      if (t > 0)
        dU_g += delta_g * h[t - 1].transpose();
      db_g += delta_g.col(0);
    }

    // // SDG updating
    // W_i -= lr * dW_i;
    // U_i -= lr * dU_i;
    // b_i -= lr * db_i;
    // W_f -= lr * dW_f;
    // U_f -= lr * dU_f;
    // b_f -= lr * db_f;
    // W_g -= lr * dW_g;
    // U_g -= lr * dU_g;
    // b_g -= lr * db_g;
    // W_o -= lr * dW_o;
    // U_o -= lr * dU_o;
    // b_o -= lr * db_o;

    // ADAM updating
    t_adam++;
    auto adam_update = [this](MatrixXd &param, MatrixXd &m, MatrixXd &v, const MatrixXd &grad) {
      m = beta1 * m + (1 - beta1) * grad;
      v = beta2 * v + (1 - beta2) * grad.array().square().matrix();
      MatrixXd m_hat = m / (1 - pow(beta1, t_adam));
      MatrixXd v_hat = v / (1 - pow(beta2, t_adam));
      MatrixXd update = lr * m_hat.array() / (v_hat.array().sqrt() + epsilon).array();
      param = param - update;
    };
    auto adam_update_vec = [this](VectorXd &param, VectorXd &m, VectorXd &v, const VectorXd &grad) {
      m = beta1 * m + (1 - beta1) * grad;
      v = beta2 * v + (1 - beta2) * grad.array().square().matrix();
      VectorXd m_hat = m / (1 - pow(beta1, t_adam));
      VectorXd v_hat = v / (1 - pow(beta2, t_adam));
      VectorXd update = lr * m_hat.array() / (v_hat.array().sqrt() + epsilon).array();
      param = param - update;
    };
    adam_update(W_i, m_W_i, v_W_i, dW_i);
    adam_update(U_i, m_U_i, v_U_i, dU_i);
    adam_update_vec(b_i, m_b_i, v_b_i, db_i);
    adam_update(W_f, m_W_f, v_W_f, dW_f);
    adam_update(U_f, m_U_f, v_U_f, dU_f);
    adam_update_vec(b_f, m_b_f, v_b_f, db_f);
    adam_update(W_o, m_W_o, v_W_o, dW_o);
    adam_update(U_o, m_U_o, v_U_o, dU_o);
    adam_update_vec(b_o, m_b_o, v_b_o, db_o);
    adam_update(W_g, m_W_g, v_W_g, dW_g);
    adam_update(U_g, m_U_g, v_U_g, dU_g);
    adam_update_vec(b_g, m_b_g, v_b_g, db_g);
  }
};

int main() {
  // AirPassengers 数据
  std::vector<double> data = {
      112, 118, 132, 129, 121, 135, 148, 148, 136, 119, 104, 118, 115, 126, 141, 135, 125, 149,
      170, 170, 158, 133, 114, 140, 145, 150, 178, 163, 172, 178, 199, 199, 184, 162, 146, 166,
      171, 180, 193, 181, 183, 218, 230, 242, 209, 191, 172, 194, 196, 196, 236, 235, 229, 243,
      264, 272, 237, 211, 180, 201, 204, 188, 235, 227, 234, 264, 302, 293, 259, 229, 203, 229,
      242, 233, 267, 269, 270, 315, 364, 347, 312, 274, 237, 278, 284, 277, 317, 313, 318, 374,
      413, 405, 355, 306, 271, 306, 315, 301, 356, 348, 355, 422, 465, 467, 404, 347, 305, 336,
      340, 318, 362, 348, 363, 435, 491, 505, 404, 359, 310, 337, 360, 342, 406, 396, 420, 472,
      548, 559, 463, 407, 362, 405, 417, 391, 419, 461, 472, 535, 622, 606, 508, 461, 390, 432};

  const double max_val = *std::ranges::max_element(data);
  const double min_val = *std::ranges::min_element(data);
  for (auto &v : data)
    v = (v - min_val) / (max_val - min_val);

  constexpr int input_size = 1;
  constexpr int seq_size = 12; // 用过去12月作为输入
  constexpr int hidden_size = 10;
  constexpr double lr = 0.01;
  LSTM lstm(input_size, hidden_size, lr);

  constexpr int epochs = 300;
  const int train_len = static_cast<int>(data.size()) - 12;

  MatrixXd W_y = MatrixXd::Random(1, hidden_size) * 0.1;
  MatrixXd b_y = MatrixXd::Zero(1, 1);

  for (int e = 0; e < epochs; e++) {
    double loss = 0.0;
    for (int t = 0; t < train_len; t++) {
      // 构建输入 x = 过去12个月
      MatrixXd x(seq_size, 1);
      for (int k = 0; k < seq_size; k++) {
        x(k, 0) = data[t + k];
      }
      MatrixXd y(1, 1);
      y(0, 0) = data[t + seq_size]; // 预测下一个月

      // 前向传播
      lstm.forward(x);

      // 输出层前向
      MatrixXd y_pred = W_y * lstm.h[seq_size - 1] + b_y;
      MatrixXd dy = y_pred - y;
      loss += dy(0, 0) * dy(0, 0);

      // 反向传播到隐藏层
      MatrixXd dWy = dy * lstm.h[seq_size - 1].transpose();
      MatrixXd delta_h = W_y.transpose() * dy;

      lstm.backward_with_dh(x, delta_h);

      // 输出层梯度更新
      W_y -= lr * dWy;
      b_y -= lr * dy;
    }
    if (e % 50 == 0)
      std::cout << "Epoch " << e << ", Loss: " << loss / train_len << std::endl;
  }

  // 使用滑动窗口预测整个序列
  std::vector<double> predictions(data.size() - seq_size);
  std::vector<double> true_values(data.size() - seq_size);
  for (int t = 0; t < train_len; t++) {
    MatrixXd x(seq_size, 1);
    for (int k = 0; k < seq_size; k++)
      x(k, 0) = data[t + k];

    lstm.forward(x);

    const double y_pred = (W_y * lstm.h[seq_size - 1] + b_y)(0, 0);
    predictions[t] = y_pred * (max_val - min_val) + min_val; // 反归一化
  }

  // 计算 RMSE 与 MAD
  double rmse = 0.0, mad = 0.0;
  for (int t = 0; t < train_len; t++) {
    const double true_val = data[t + seq_size] * (max_val - min_val) + min_val;
    const double error = predictions[t] - true_val;
    true_values[t] = true_val;
    rmse += error * error;
    mad += fabs(error);
    std::cout << "idx=" << t << " true = " << true_val << " pred = " << predictions[t] << std::endl;
  }
  rmse = sqrt(rmse / train_len);
  std::cout << std::endl;
  std::cout << "Total absolute error " << mad << std::endl;
  mad /= train_len;

  std::cout << "RMSE: " << rmse << ", MAE(mean absolute error): " << mad << std::endl;

  // 画图
  draw_pic(predictions, true_values);

  return 0;
}
