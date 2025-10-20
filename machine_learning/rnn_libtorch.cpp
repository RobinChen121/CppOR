/*
 * Created by Zhen Chen on 2025/10/20.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include <iostream>
#include <torch/torch.h>
#include <vector>

// 定义一个简单的 RNN 模型
struct RNNModel : torch::nn::Module {
  torch::nn::RNN rnn{nullptr};
  torch::nn::Linear fc{nullptr};

  RNNModel(int input_size, int hidden_size, int output_size) {
    rnn = register_module(
        "rnn", torch::nn::RNN(torch::nn::RNNOptions(input_size, hidden_size).batch_first(true)));
    fc = register_module("fc", torch::nn::Linear(hidden_size, output_size));
  }

  torch::Tensor forward(torch::Tensor x) {
    auto rnn_out = rnn->forward(x);
    auto h_out = std::get<0>(rnn_out);                     // 取所有时间步的输出
    auto last_output = h_out.select(1, h_out.size(1) - 1); // 取最后一个时间步
    return fc->forward(last_output);
  }
};

int main() {
  // 参数定义
  int input_size = 1;
  int hidden_size = 16;
  int output_size = 1;
  int sequence_length = 10;

  // 构建模型
  RNNModel model(input_size, hidden_size, output_size);

  // 优化器
  torch::optim::Adam optimizer(model.parameters(), torch::optim::AdamOptions(0.01));

  // 模拟训练数据: y = x + 1
  std::vector<float> data(100);
  for (int i = 0; i < 100; ++i)
    data[i] = i;

  for (int epoch = 0; epoch < 200; ++epoch) {
    float loss_sum = 0.0;

    for (int i = 0; i < 90; ++i) {
      // 构造输入序列
      std::vector<float> input_seq(data.begin() + i, data.begin() + i + sequence_length);
      float target = data[i + sequence_length];

      auto x = torch::tensor(input_seq).reshape({1, sequence_length, 1});
      auto y = torch::tensor({target}).reshape({1, 1});

      // 前向 + 反向
      optimizer.zero_grad();
      auto output = model.forward(x);
      auto loss = torch::mse_loss(output, y);
      loss.backward();
      optimizer.step();

      loss_sum += loss.item<float>();
    }

    if (epoch % 20 == 0)
      std::cout << "Epoch [" << epoch << "] Loss: " << loss_sum / 90.0 << std::endl;
  }

  // 测试
  const std::vector<float> test_seq = {90, 91, 92, 93, 94, 95, 96, 97, 98, 99};
  const auto test_x = torch::tensor(test_seq).reshape({1, sequence_length, 1});
  const auto pred = model.forward(test_x);
  std::cout << "Predicted next value: " << pred.item<float>() << std::endl;

  return 0;
}
