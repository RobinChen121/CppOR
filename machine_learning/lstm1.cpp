/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 01/11/2025, 14:13
 * Description: 这个代码只有单个时间步
 *
 */

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

using namespace Eigen;
using namespace std;

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

  // 隐藏状态
  MatrixXd h;
  MatrixXd c;

  LSTM(int input_size_, int hidden_size_, double lr_ = 0.001)
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

    h = MatrixXd::Zero(hidden_size, 1);
    c = MatrixXd::Zero(hidden_size, 1);

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

  void forward(const MatrixXd &x, MatrixXd &i_t, MatrixXd &f_t, MatrixXd &o_t, MatrixXd &g_t,
               MatrixXd &c_t, MatrixXd &h_t) {
    i_t = sigmoid(W_i * x + U_i * h + b_i);
    f_t = sigmoid(W_f * x + U_f * h + b_f);
    o_t = sigmoid(W_o * x + U_o * h + b_o);
    g_t = (W_g * x + U_g * h + b_g).array().tanh();

    c = f_t.array() * c.array() + i_t.array() * g_t.array();
    h = o_t.array() * c.array().tanh();

    c_t = c;
    h_t = h;
  }

  void backward_with_dh(const MatrixXd &x, const MatrixXd &dh_external) {
    MatrixXd i_t(hidden_size, 1), f_t(hidden_size, 1), o_t(hidden_size, 1), g_t(hidden_size, 1),
        c_t(hidden_size, 1), h_t(hidden_size, 1);
    forward(x, i_t, f_t, o_t, g_t, c_t, h_t);

    MatrixXd dh = dh_external;
    MatrixXd dc = dh.array() * o_t.array() * (1 - c_t.array().tanh().square());

    MatrixXd di = dc.array() * g_t.array() * i_t.array() * (1 - i_t.array());
    MatrixXd df = dc.array() * c.array() * f_t.array() * (1 - f_t.array());
    MatrixXd do_ = dh.array() * c_t.array().tanh() * o_t.array() * (1 - o_t.array());
    MatrixXd dg = dc.array() * i_t.array() * (1 - g_t.array().square());

    MatrixXd dW_i = di * x.transpose();
    MatrixXd dU_i = di * h.transpose();
    VectorXd db_i = di.col(0);
    MatrixXd dW_f = df * x.transpose();
    MatrixXd dU_f = df * h.transpose();
    VectorXd db_f = df.col(0);
    MatrixXd dW_o = do_ * x.transpose();
    MatrixXd dU_o = do_ * h.transpose();
    VectorXd db_o = do_.col(0);
    MatrixXd dW_g = dg * x.transpose();
    MatrixXd dU_g = dg * h.transpose();
    VectorXd db_g = dg.col(0);

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
  vector<double> data = {
      112, 118, 132, 129, 121, 135, 148, 148, 136, 119, 104, 118, 115, 126, 141, 135, 125, 149,
      170, 170, 158, 133, 114, 140, 145, 150, 178, 163, 172, 178, 199, 199, 184, 162, 146, 166,
      171, 180, 193, 181, 183, 218, 230, 242, 209, 191, 172, 194, 196, 196, 236, 235, 229, 243,
      264, 272, 237, 211, 180, 201, 204, 188, 235, 227, 234, 264, 302, 293, 259, 229, 203, 229,
      242, 233, 267, 269, 270, 315, 364, 347, 312, 274, 237, 278, 284, 277, 317, 313, 318, 374,
      413, 405, 355, 306, 271, 306, 315, 301, 356, 348, 355, 422, 465, 467, 404, 347, 305, 336,
      340, 318, 362, 348, 363, 435, 491, 505, 404, 359, 310, 337, 360, 342, 406, 396, 420, 472,
      548, 559, 463, 407, 362, 405, 417, 391, 419, 461, 472, 535, 622, 606, 508, 461, 390, 432};

  double max_val = *ranges::max_element(data);
  double min_val = *ranges::min_element(data);
  for (auto &v : data)
    v = (v - min_val) / (max_val - min_val);

  int input_size = 12; // 用过去12月作为输入
  int hidden_size = 10;
  double lr = 0.001;
  LSTM lstm(input_size, hidden_size, lr);

  MatrixXd W_y = MatrixXd::Random(1, hidden_size) * 0.1;
  MatrixXd b_y = MatrixXd::Zero(1, 1);

  int epochs = 500;
  int train_len = data.size() - 12;

  for (int e = 0; e < epochs; e++) {
    double loss = 0.0;
    for (int t = 0; t < train_len; t++) {
      // 构建输入 x = 过去12个月
      MatrixXd x(input_size, 1);
      for (int k = 0; k < input_size; k++) {
        x(k, 0) = data[t + k];
      }
      MatrixXd y(1, 1);
      y(0, 0) = data[t + input_size]; // 预测下一个月

      // 前向传播
      MatrixXd i_dummy(hidden_size, 1), f_dummy(hidden_size, 1), o_dummy(hidden_size, 1),
          g_dummy(hidden_size, 1), c_dummy(hidden_size, 1), h_dummy(hidden_size, 1);
      lstm.forward(x, i_dummy, f_dummy, o_dummy, g_dummy, c_dummy, h_dummy);

      // 输出层前向
      MatrixXd y_pred = W_y * h_dummy + b_y;
      MatrixXd dy = y_pred - y;
      loss += dy(0, 0) * dy(0, 0);

      // 输出层梯度更新
      W_y -= lr * dy * h_dummy.transpose();
      b_y -= lr * dy;

      // 反向传播到隐藏层
      MatrixXd dh = W_y.transpose() * dy;
      lstm.backward_with_dh(x, dh);
    }
    if (e % 50 == 0)
      cout << "Epoch " << e << ", Loss: " << loss / train_len << endl;
  }

  // 使用滑动窗口预测整个序列
  vector<double> predictions(data.size());
  for (int t = 0; t < train_len; t++) {
    MatrixXd x(input_size, 1);
    for (int k = 0; k < input_size; k++)
      x(k, 0) = data[t + k];

    MatrixXd i_dummy(hidden_size, 1), f_dummy(hidden_size, 1), o_dummy(hidden_size, 1),
        g_dummy(hidden_size, 1), c_dummy(hidden_size, 1), h_dummy(hidden_size, 1);
    lstm.forward(x, i_dummy, f_dummy, o_dummy, g_dummy, c_dummy, h_dummy);

    double y_pred = (W_y * h_dummy + b_y)(0, 0);
    predictions[t + input_size] = y_pred * (max_val - min_val) + min_val; // 反归一化
  }

  // 计算 RMSE 与 MAD
  double rmse = 0.0, mad = 0.0;
  for (int t = 0; t < train_len; t++) {
    double true_val = data[t + input_size] * (max_val - min_val) + min_val;
    double error = predictions[t + input_size] - true_val;
    rmse += error * error;
    mad += fabs(error);
    std::cout << "idx=" << t << " true=" << true_val << " pred=" << predictions[t + input_size]
              << std::endl;
  }
  rmse = sqrt(rmse / train_len);
  std::cout << std::endl;
  cout << "Total absolute error " << mad << endl;
  mad /= train_len;

  cout << "RMSE: " << rmse << ", MAE(mean absolute error): " << mad << endl;

  // 预测下一月
  MatrixXd x(input_size, 1);
  int last_idx = data.size() - input_size;
  for (int k = 0; k < input_size; k++)
    x(k, 0) = data[last_idx + k];

  MatrixXd i_dummy(hidden_size, 1), f_dummy(hidden_size, 1), o_dummy(hidden_size, 1),
      g_dummy(hidden_size, 1), c_dummy(hidden_size, 1), h_dummy(hidden_size, 1);
  lstm.forward(x, i_dummy, f_dummy, o_dummy, g_dummy, c_dummy, h_dummy);
  double next_month = (W_y * h_dummy + b_y)(0, 0) * (max_val - min_val) + min_val;
  cout << "Next month prediction: " << next_month << endl;

  return 0;
}
