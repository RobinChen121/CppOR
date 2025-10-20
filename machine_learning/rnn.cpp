/*
 * Created by Zhen Chen on 2025/10/20.
 * Email: chen.zhen5526@gmail.com
 * Description: 
 *
 *
 */
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

using namespace std;

// 激活函数
double tanh_act(double x) { return tanh(x); }
double tanh_deriv(double x) { double t = tanh(x); return 1 - t*t; }

// 向量操作辅助函数
vector<double> vec_add(const vector<double>& a, const vector<double>& b) {
    vector<double> res(a.size());
    for (size_t i=0;i<a.size();i++) res[i] = a[i] + b[i];
    return res;
}

vector<double> vec_mul_scalar(const vector<double>& a, double s) {
    vector<double> res(a.size());
    for (size_t i=0;i<a.size();i++) res[i] = a[i]*s;
    return res;
}

// RNN 多隐藏单元
struct MultiRNN {
    int H; // 隐藏单元数
    vector<double> h;          // 隐藏状态
    vector<vector<double>> W_x; // H x 1
    vector<vector<double>> W_h; // H x H
    vector<double> W_y;         // 1 x H
    vector<double> b_h;         // H
    double b_y;

    MultiRNN(int hidden_size) : H(hidden_size) {
        srand(time(0));
        h.resize(H, 0.0);
        W_x.resize(H, vector<double>(1));
        W_h.resize(H, vector<double>(H));
        W_y.resize(H);
        b_h.resize(H);
        b_y = ((double)rand()/RAND_MAX -0.5);

        // 初始化权重
        for(int i=0;i<H;i++){
            W_x[i][0] = ((double)rand()/RAND_MAX -0.5);
            W_y[i] = ((double)rand()/RAND_MAX -0.5);
            b_h[i] = ((double)rand()/RAND_MAX -0.5);
            for(int j=0;j<H;j++){
                W_h[i][j] = ((double)rand()/RAND_MAX -0.5);
            }
        }
    }

    // 前向传播
    double forward(double x_t) {
        vector<double> h_new(H, 0.0);
        for(int i=0;i<H;i++){
            double sum = W_x[i][0]*x_t + b_h[i];
            for(int j=0;j<H;j++){
                sum += W_h[i][j]*h[j];
            }
            h_new[i] = tanh_act(sum);
        }
        h = h_new;

        double y = 0.0;
        for(int i=0;i<H;i++) y += W_y[i]*h[i];
        y += b_y;
        return y;
    }
};

int main() {
    vector<double> inputs = {1,2,3,4,5,6,7,8,9,10};
    vector<double> targets = {2,3,4,5,6,7,8,9,10,11};

    MultiRNN rnn(3); // 3 个隐藏单元
    double lr = 0.01;
    int epochs = 5000;

    for(int epoch=0; epoch<epochs; epoch++){
        double total_loss = 0.0;
        vector<vector<double>> hs(inputs.size(), vector<double>(3));
        vector<double> outputs(inputs.size());
        vector<vector<double>> pre_acts(inputs.size(), vector<double>(3));

        // 前向传播
        vector<double> h_prev(3,0.0);
        for(size_t t=0;t<inputs.size();t++){
            double x = inputs[t];
            vector<double> h_new(3,0.0);
            for(int i=0;i<3;i++){
                double sum = rnn.W_x[i][0]*x + rnn.b_h[i];
                for(int j=0;j<3;j++) sum += rnn.W_h[i][j]*h_prev[j];
                pre_acts[t][i] = sum;
                h_new[i] = tanh_act(sum);
            }
            rnn.h = h_new;
            hs[t] = h_new;
            h_prev = h_new;

            double y = 0.0;
            for(int i=0;i<3;i++) y += rnn.W_y[i]*h_new[i];
            y += rnn.b_y;
            outputs[t] = y;
            total_loss += 0.5*(y-targets[t])*(y-targets[t]);
        }

        // BPTT（简化版，逐步更新）
        vector<double> dh_next(3,0.0);
        vector<double> dW_y(3,0.0);
        double db_y = 0.0;
        vector<double> db_h(3,0.0);
        vector<vector<double>> dW_x(3, vector<double>(1,0.0));
        vector<vector<double>> dW_h(3, vector<double>(3,0.0));

        for(int t=inputs.size()-1;t>=0;t--){
            double dy = outputs[t]-targets[t];
            for(int i=0;i<3;i++){
                dW_y[i] += dy*hs[t][i];
            }
            db_y += dy;

            vector<double> dh(3,0.0);
            for(int i=0;i<3;i++){
                dh[i] = rnn.W_y[i]*dy + dh_next[i];
            }

            vector<double> dpre(3,0.0);
            for(int i=0;i<3;i++){
                dpre[i] = dh[i]*tanh_deriv(pre_acts[t][i]);
                db_h[i] += dpre[i];
                dW_x[i][0] += dpre[i]*inputs[t];
                for(int j=0;j<3;j++){
                    double h_prev_val = (t==0)?0.0:hs[t-1][j];
                    dW_h[i][j] += dpre[i]*h_prev_val;
                }
            }

            for(int i=0;i<3;i++){
                double sum = 0.0;
                for(int j=0;j<3;j++) sum += rnn.W_h[j][i]*dpre[j];
                dh_next[i] = sum;
            }
        }

        // 更新参数
        for(int i=0;i<3;i++){
            rnn.W_y[i] -= lr*dW_y[i];
            rnn.b_h[i] -= lr*db_h[i];
            rnn.W_x[i][0] -= lr*dW_x[i][0];
            for(int j=0;j<3;j++) rnn.W_h[i][j] -= lr*dW_h[i][j];
        }
        rnn.b_y -= lr*db_y;

        if(epoch%200==0) cout << "Epoch "<<epoch<<" | Loss: "<<total_loss/inputs.size()<<endl;
    }

    // 测试
    double test_input = 10.0;
    rnn.h = vector<double>(3,0.0);
    for(double x:inputs) rnn.forward(x); // 喂历史序列
    double pred = rnn.forward(test_input);
    cout << "\n预测 y(6) = "<<pred<<endl;

    return 0;
}
