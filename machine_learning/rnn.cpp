/*
 * Created by Zhen Chen on 2025/10/20.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */
#include "../utils/matplotlibcpp.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

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

// ---------------------------- utilities ----------------------------
double uniform_rand(const double a, const double b) {
  static std::random_device rd;  // 真随机种子
  static std::mt19937 gen(rd()); // rd()                           // 梅森旋转算法引擎
  static std::uniform_real_distribution<double> dist(a, b); // 区间 [a,b]
  return dist(gen);
}

std::vector<double> slice(const std::vector<double> &v, int i, int len) {
  return std::vector<double>(v.begin() + i, v.begin() + i + len);
}

// ---------------------------- activation ----------------------------
inline double tanh_act(double x) { return std::tanh(x); }
inline double tanh_deriv(double y) { // input is y = tanh(x), derivative = 1 - y^2
  return 1.0 - y * y;
}

// ---------------------------- RNN model ----------------------------
// Vanilla RNN: hidden_t = tanh(Wxh * x_t + Whh * hidden_{t-1} + bh)
// output   = tanh( Why * hidden_T + by )  (we produce output at final time step)
struct RNN {
  int input_dim;
  int hidden_dim;
  int output_dim;

  // weights and biases
  std::vector<std::vector<double>> Wxh; // hidden_dim x input_dim
  std::vector<std::vector<double>> Whh; // hidden_dim x hidden_dim
  std::vector<std::vector<double>> Why; // output_dim x hidden_dim
  std::vector<double> bh;               // hidden_dim
  std::vector<double> by;               // output_dim

  // Adam 动量参数
  std::vector<std::vector<double>> mWxh, vWxh;
  std::vector<std::vector<double>> mWhh, vWhh;
  std::vector<std::vector<double>> mWhy, vWhy;
  std::vector<double> mbh, vbh, mby, vby;

  int t = 0; // 全局迭代计数

  // constructor: random init
  RNN(const int in, const int hid, const int out)
      : input_dim(in), hidden_dim(hid), output_dim(out) {
    Wxh.assign(hid, std::vector<double>(in));
    Whh.assign(hid, std::vector<double>(hid));
    Why.assign(out, std::vector<double>(hid));
    bh.assign(hid, 0.0);
    by.assign(out, 0.0);

    constexpr double scale = 0.08;
    for (int i = 0; i < hid; ++i)
      for (int j = 0; j < in; ++j)
        Wxh[i][j] = uniform_rand(-scale, scale);
    for (int i = 0; i < hid; ++i)
      for (int j = 0; j < hid; ++j)
        Whh[i][j] = uniform_rand(-scale, scale);
    for (int i = 0; i < out; ++i)
      for (int j = 0; j < hid; ++j)
        Why[i][j] = uniform_rand(-scale, scale);
    for (int i = 0; i < hid; ++i)
      bh[i] = 0.0;
    for (int i = 0; i < out; ++i)
      by[i] = 0.0;

    // Adam 动量参数初始化
    mWxh.assign(hid, std::vector<double>(in));
    vWxh.assign(hid, std::vector<double>(in));
    mWhh.assign(hid, std::vector<double>(hid));
    vWhh.assign(hid, std::vector<double>(hid));
    mWhy.assign(out, std::vector<double>(hid));
    vWhy.assign(out, std::vector<double>(hid));
    mbh.assign(hid, 0.0);
    mby.assign(out, 0.0);
    vbh.assign(hid, 0.0);
    vby.assign(out, 0.0);
  }

  // forward for one input sequence x_seq of length T (each x_t is scalar here)
  // returns output scalar and also stores intermediate z_h and h for BPTT
  double forward(const std::vector<double> &x_seq, std::vector<std::vector<double>> &z_h,
                 std::vector<std::vector<double>> &h, std::vector<std::vector<double>> &z_y,
                 std::vector<std::vector<double>> &y_hat) const {
    const int T = static_cast<int>(x_seq.size());
    z_h.assign(T, std::vector(hidden_dim, 0.0));
    h.assign(T, std::vector(hidden_dim, 0.0));
    z_y.assign(T, std::vector(output_dim, 0.0));
    y_hat.assign(T, std::vector(output_dim, 0.0));

    std::vector h_prev(hidden_dim, 0.0);

    for (int t = 0; t < T; ++t) { // T is the number of sequence
      // z_h[t] = Wxh * x_t + Whh * h_prev + bh
      for (int i = 0; i < hidden_dim; ++i) {
        double s = bh[i];
        s += Wxh[i][0] * x_seq[t]; // input_dim == 1
        for (int j = 0; j < hidden_dim; ++j)
          s += Whh[i][j] * h_prev[j];
        z_h[t][i] = s;
        h[t][i] = tanh_act(s);
      }
      // output at this time step
      for (int k = 0; k < output_dim; ++k) {
        double s = by[k];
        for (int i = 0; i < hidden_dim; ++i)
          s += Why[k][i] * h[t][i];
        z_y[t][k] = s;
        y_hat[t][k] = tanh_act(s); // output activation
      }
      h_prev = h[t];
    }
    // return last time step output scalar (output_dim == 1)
    return y_hat[T - 1][0];
  }

  // compute gradients via BPTT for a single sequence (x_seq) and target y_true (scalar)
  // using formulas you provided: delta_y_t = (y_hat_t - y_t) * (1 - y_hat_t^2)
  // delta_h_t = (Why^T * delta_y_t + Whh^T * delta_h_{t+1}) .* f'(z_h_t)
  void bptt(const std::vector<double> &x_seq, const double y_true,
            const std::vector<std::vector<double>> &z_h, const std::vector<std::vector<double>> &h,
            const std::vector<std::vector<double>> &z_y,
            const std::vector<std::vector<double>> &y_hat,
            // outputs (gradients)
            std::vector<std::vector<double>> &dWxh, std::vector<std::vector<double>> &dWhh,
            std::vector<std::vector<double>> &dWhy, std::vector<double> &dbh,
            std::vector<double> &dby) const {

    const int T = static_cast<int>(x_seq.size());
    // zero gradients
    for (int i = 0; i < hidden_dim; ++i) {
      for (int j = 0; j < input_dim; ++j)
        dWxh[i][j] = 0.0;
      for (int j = 0; j < hidden_dim; ++j)
        dWhh[i][j] = 0.0;
      dbh[i] = 0.0;
    }
    for (int i = 0; i < output_dim; ++i) {
      for (int j = 0; j < hidden_dim; ++j)
        dWhy[i][j] = 0.0;
      dby[i] = 0.0;
    }

    // We must compute delta_y_t for all t (but only final target matters if we train many-to-one).
    // Here we treat the target as the next value after the last input; so only L_T is nonzero.
    // However, we'll compute delta_y for every time step, with target only at final step.
    std::vector<std::vector<double>> delta_y(T, std::vector<double>(output_dim, 0.0));
    for (int t = 0; t < T; ++t) {
      if (t == T - 1) {
        // L_t = 0.5 * (y_true - y_hat[t])^2 -> dL/dy_hat = (y_hat - y_true)
        // since output dimension is 1
        delta_y[t][0] = (y_hat[t][0] - y_true) * (1.0 - y_hat[t][0] * y_hat[t][0]);
      } else {
        delta_y[t][0] = 0.0; // intermediate steps have no direct target
      }
    }

    // Backpropagation through time for hidden deltas
    std::vector delta_h(T, std::vector(hidden_dim, 0.0));
    // delta_h_{T} depends on delta_y[T] and delta_h_{T+1}=0
    std::vector delta_h_next(hidden_dim, 0.0);

    for (int t = T - 1; t >= 0; --t) {
      // compute W_hy^T * delta_y_t  (hidden_dim vector)
      std::vector term1(hidden_dim, 0.0);
      for (int i = 0; i < hidden_dim; ++i) {
        term1[i] = 0.0;
        for (int k = 0; k < output_dim; ++k)
          term1[i] += Why[k][i] * delta_y[t][k];
      }
      // compute W_hh^T * delta_h_next
      std::vector<double> term2(hidden_dim, 0.0);
      for (int i = 0; i < hidden_dim; ++i) {
        term2[i] = 0.0;
        for (int j = 0; j < hidden_dim; ++j)
          term2[i] += Whh[j][i] * delta_h_next[j];
      }
      for (int i = 0; i < hidden_dim; ++i) {
        double pre = term1[i] + term2[i];
        double local = tanh_deriv(h[t][i]); // f'(z_h_t) = 1 - h_t^2, using h[t][i] = tanh(z)
        delta_h[t][i] = pre * local;
      }

      // accumulate gradients:
      // dWhy += delta_y[t] * h[t]^T
      for (int k = 0; k < output_dim; ++k)
        for (int i = 0; i < hidden_dim; ++i)
          dWhy[k][i] += delta_y[t][k] * h[t][i];
      for (int k = 0; k < output_dim; ++k)
        dby[k] += delta_y[t][k];

      // dWxh += delta_h[t] * x_t^T  (input_dim==1)
      for (int i = 0; i < hidden_dim; ++i)
        dWxh[i][0] += delta_h[t][i] * x_seq[t];

      // dWhh += delta_h[t] * h[t-1]^T
      for (int i = 0; i < hidden_dim; ++i) {
        for (int j = 0; j < hidden_dim; ++j) {
          double h_prev = (t > 0) ? h[t - 1][j] : 0.0;
          dWhh[i][j] += delta_h[t][i] * h_prev;
        }
      }
      for (int i = 0; i < hidden_dim; ++i)
        dbh[i] += delta_h[t][i];

      // shift
      delta_h_next = delta_h[t];
    }
  }

  // apply gradients with simple SGD (in-place)
  void SDG(const std::vector<std::vector<double>> &dWxh,
           const std::vector<std::vector<double>> &dWhh,
           const std::vector<std::vector<double>> &dWhy, const std::vector<double> &dbh,
           const std::vector<double> &dby, const double lr = 0.01, const double clip = 5.0) {
    // gradient clipping (element-wise)
    // 作用是把梯度限制在 [-clip, clip] 的范围内
    // 防止梯度爆炸 的常用手段，尤其在 RNN / LSTM 中很常见
    auto clip_val = [&](const double v) -> double {
      if (v > clip)
        return clip;
      if (v < -clip)
        return -clip;
      return v;
    };

    for (int i = 0; i < hidden_dim; ++i) {
      for (int j = 0; j < input_dim; ++j)
        Wxh[i][j] -= lr * clip_val(dWxh[i][j]);
      for (int j = 0; j < hidden_dim; ++j)
        Whh[i][j] -= lr * clip_val(dWhh[i][j]);
      bh[i] -= lr * clip_val(dbh[i]);
    }
    for (int k = 0; k < output_dim; ++k) {
      for (int i = 0; i < hidden_dim; ++i)
        Why[k][i] -= lr * clip_val(dWhy[k][i]);
      by[k] -= lr * clip_val(dby[k]);
    }
  }

  // apply gradients with simple SGD (in-place)
  // 假设 mWxh, vWxh 等已经定义为全局或类成员，并初始化为 0
  // t 是时间步计数，从 1 开始，每次调用 Adam 更新递增
  void ADAM(const std::vector<std::vector<double>> &dWxh,
            const std::vector<std::vector<double>> &dWhh,
            const std::vector<std::vector<double>> &dWhy, const std::vector<double> &dbh,
            const std::vector<double> &dby, const double lr = 0.001, const double beta1 = 0.9,
            const double beta2 = 0.999, const double epsilon = 1e-8, const double clip = 5.0) {

    t += 1; // 全局时间步

    auto clip_val = [&](const double v) -> double {
      if (v > clip)
        return clip;
      if (v < -clip)
        return -clip;
      return v;
    };

    // 更新 Wxh
    for (int i = 0; i < hidden_dim; ++i) {
      for (int j = 0; j < input_dim; ++j) {
        const double g = clip_val(dWxh[i][j]);
        mWxh[i][j] = beta1 * mWxh[i][j] + (1 - beta1) * g;
        vWxh[i][j] = beta2 * vWxh[i][j] + (1 - beta2) * g * g;

        const double m_hat = mWxh[i][j] / (1 - pow(beta1, t));
        const double v_hat = vWxh[i][j] / (1 - pow(beta2, t));

        Wxh[i][j] -= lr * m_hat / (sqrt(v_hat) + epsilon);
      }
      // bh 更新类似
      const double g_bh = clip_val(dbh[i]);
      mbh[i] = beta1 * mbh[i] + (1 - beta1) * g_bh;
      vbh[i] = beta2 * vbh[i] + (1 - beta2) * g_bh * g_bh;
      const double m_hat_bh = mbh[i] / (1 - pow(beta1, t));
      const double v_hat_bh = vbh[i] / (1 - pow(beta2, t));
      bh[i] -= lr * m_hat_bh / (sqrt(v_hat_bh) + epsilon);

      // Whh 更新同 Wxh
      for (int j = 0; j < hidden_dim; ++j) {
        const double g = clip_val(dWhh[i][j]);
        mWhh[i][j] = beta1 * mWhh[i][j] + (1 - beta1) * g;
        vWhh[i][j] = beta2 * vWhh[i][j] + (1 - beta2) * g * g;
        const double m_hat = mWhh[i][j] / (1 - pow(beta1, t));
        const double v_hat = vWhh[i][j] / (1 - pow(beta2, t));
        Whh[i][j] -= lr * m_hat / (sqrt(v_hat) + epsilon);
      }
    }

    // Why, by 更新同上，逻辑一样
    for (int k = 0; k < output_dim; ++k) {
      for (int i = 0; i < hidden_dim; ++i) {
        const double g = clip_val(dWhy[k][i]);
        mWhy[k][i] = beta1 * mWhy[k][i] + (1 - beta1) * g;
        vWhy[k][i] = beta2 * vWhy[k][i] + (1 - beta2) * g * g;
        const double m_hat = mWhy[k][i] / (1 - pow(beta1, t));
        const double v_hat = vWhy[k][i] / (1 - pow(beta2, t));
        Why[k][i] -= lr * m_hat / (sqrt(v_hat) + epsilon);
      }
      const double g_by = clip_val(dby[k]);
      mby[k] = beta1 * mby[k] + (1 - beta1) * g_by;
      vby[k] = beta2 * vby[k] + (1 - beta2) * g_by * g_by;
      const double m_hat_by = mby[k] / (1 - pow(beta1, t));
      const double v_hat_by = vby[k] / (1 - pow(beta2, t));
      by[k] -= lr * m_hat_by / (sqrt(v_hat_by) + epsilon);
    }
  }
};

// ---------------------------- main: load data, train, test ----------------------------
int main() {
  // AirPassengers monthly data (Jan 1949 - Dec 1960) 144 months
  // Source values hard-coded (integers): classic dataset
  std::vector<double> data = {
      112, 118, 132, 129, 121, 135, 148, 148, 136, 119, 104, 118, 115, 126, 141, 135, 125, 149,
      170, 170, 158, 133, 114, 140, 145, 150, 178, 163, 172, 178, 199, 199, 184, 162, 146, 166,
      171, 180, 193, 181, 183, 218, 230, 242, 209, 191, 172, 194, 196, 196, 236, 235, 229, 243,
      264, 272, 237, 211, 180, 201, 204, 188, 235, 227, 234, 264, 302, 293, 259, 229, 203, 229,
      242, 233, 267, 269, 270, 315, 364, 347, 312, 274, 237, 278, 284, 277, 317, 313, 318, 374,
      413, 405, 355, 306, 271, 306, 315, 301, 356, 348, 355, 422, 465, 467, 404, 347, 305, 336,
      340, 318, 362, 348, 363, 435, 491, 505, 404, 359, 310, 337, 360, 342, 406, 396, 420, 472,
      548, 559, 463, 407, 362, 405, 417, 391, 419, 461, 472, 535, 622, 606, 508, 461, 390, 432};
  int N = static_cast<int>(data.size()); // 144
  // Normalize into [-1,1] via min-max
  double min_data = *std::ranges::min_element(data); // from algorithm
  double max_data = *std::ranges::max_element(data);

  auto normalize = [&](const double x) -> double { // lambda function
    // scale to [-1,1]
    return -1.0 + 2.0 * (x - min_data) / (max_data - min_data);
  };
  auto denormalize = [&](const double y) -> double { // lambda function
    // inverse map from [-1,1] back to original
    return min_data + (y + 1.0) * (max_data - min_data) / 2.0;
  };

  std::vector<double> series(N);
  for (int i = 0; i < N; ++i)
    series[i] = normalize(data[i]);

  // Settings
  int seq_len = 12; // use past 12 months to predict next month
  int hidden_dim = 50;
  int epochs = 300;
  double test_ratio = 0.2; // last 20% as test
  int test_start = static_cast<int>(N * (1.0 - test_ratio));

  // Prepare training samples: sliding windows (x: seq_len values -> y: next value)
  std::vector<std::vector<double>> X;
  std::vector<double> Y;
  // every time input a sequence of data, in a rolling manner
  for (int i = 0; i + seq_len < N; ++i) {
    std::vector x(series.begin() + i, series.begin() + i + seq_len);
    double y = series[i + seq_len];
    X.push_back(x);
    Y.push_back(y);
  }
  int total_samples = static_cast<int>(X.size()); // N - seq_len

  int train_samples = 0;
  for (int i = 0; i < total_samples; ++i) {
    // check if sample's prediction index (i+seq_len) is < test_start
    if (i + seq_len < test_start)
      train_samples++;
  }
  int test_samples = total_samples - train_samples;

  std::cout << "Total samples: " << total_samples << ", train: " << train_samples
            << ", val: " << test_samples << std::endl;
  std::cout << "Hidden dim: " << hidden_dim << ", seq_len: " << seq_len << ", epochs: " << epochs
            << std::endl;

  // Initialize RNN
  // 这个 1 是指每个时间步 t 输入 1 个数据
  RNN rnn(1, hidden_dim, 1); // 每个时间 t 可能有多个隐含层

  // gradient containers
  std::vector dWxh(hidden_dim, std::vector<double>(1));
  std::vector dWhh(hidden_dim, std::vector<double>(hidden_dim));
  std::vector dWhy(1, std::vector<double>(hidden_dim));
  std::vector<double> dbh(hidden_dim), dby(1);

  // training loop
  for (int ep = 1; ep <= epochs; ++ep) {
    double epoch_loss = 0.0;
    // simple epoch over training samples in order (no shuffling because time series)
    for (int idx = 0; idx < train_samples; ++idx) {
      const std::vector<double> &x = X[idx];
      double y_true = Y[idx];

      std::vector<std::vector<double>> z_h, h, z_y, y_hat;
      double y_pred = rnn.forward(x, z_h, h, z_y, y_hat);

      // compute loss (MSE)
      double loss = 0.5 * (y_pred - y_true) * (y_pred - y_true);
      epoch_loss += loss;

      // compute gradients by BPTT
      rnn.bptt(x, y_true, z_h, h, z_y, y_hat, dWxh, dWhh, dWhy, dbh, dby);

      double lr = 0.001;

      // apply SGD update
      // rnn.SDG(dWxh, dWhh, dWhy, dbh, dby, lr);

      // apply ADAM update
      rnn.ADAM(dWxh, dWhh, dWhy, dbh, dby, lr);
    }

    epoch_loss /= std::max(1, train_samples);
    if (ep % 25 == 0 || ep == 1 || ep == epochs) {
      // evaluate validation loss
      double val_loss = 0.0;
      for (int idx = train_samples; idx < total_samples; ++idx) {
        std::vector<std::vector<double>> z_h, h, z_y, y_hat;
        double y_pred = rnn.forward(X[idx], z_h, h, z_y, y_hat);
        double l = 0.5 * (y_pred - Y[idx]) * (y_pred - Y[idx]);
        val_loss += l;
      }
      val_loss /= std::max(1, test_samples);
      std::cout << "Epoch " << ep << " train_loss=" << epoch_loss << " val_loss=" << val_loss
                << std::endl;
    }
  }

  // After training: run rolling predictions on test region (one-step ahead)
  std::cout << "\n=== Test / predictions (denormalized) ===\n";
  double mse_test = 0.0;
  double mae_test = 0.0;
  int test_num = 0;
  std::vector<double> predictions, true_values;
  for (int idx = 0; idx < total_samples; ++idx) {
    std::vector<std::vector<double>> z_h, h, z_y, y_hat;
    double y_pred = rnn.forward(X[idx], z_h, h, z_y, y_hat);
    double y_true = Y[idx];

    double pred_denorm = denormalize(y_pred);
    double true_denorm = denormalize(y_true);

    predictions.push_back(pred_denorm);
    true_values.push_back(true_denorm);
    std::cout << "idx=" << idx << " true=" << true_denorm << " pred=" << pred_denorm << std::endl;

    mse_test += (pred_denorm - true_denorm) * (pred_denorm - true_denorm);
    mae_test += std::abs(pred_denorm - true_denorm);
    test_num++;
  }
  if (test_num > 0) {
    std::cout << "RMSE (denorm) = " << sqrt(mse_test / test_num) << "\n";
    std::cout << "MAE (denorm) = " << mae_test / test_num << "\n";
    std::cout << "Total absolute error (denorm) = " << mae_test << "\n";
    draw_pic(predictions, true_values);
  } else {
    std::cout << "No test samples.\n";
  }

  return 0;
}
