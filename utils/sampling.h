/*
 * Created by Zhen Chen on 2025/3/13.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#ifndef SAMPLING_H
#define SAMPLING_H

#include <iostream>
#include <vector>

// 在 C++ 中，static 成员函数是属于类本身，而不是类的对象的。
// 这意味着它们可以在不创建类实例的情况下被调用，
// 并且不能访问 非静态成员变量 或 非静态成员函数，因为它们不依赖于具体对象。
double poisson_cdf(int k, double lambda);

int poisson_quantile(double p, double lambda);
std::vector<double> generate_samples_poisson(int sample_nums, double mean);
std::vector<double> generateSamplesSelfDiscrete(const int num_samples,
                                                const std::vector<double> &values,
                                                const std::vector<double> &weights);

int rand_uniform(int a, int b);

std::vector<std::vector<int>> generate_scenario_paths(int scenario_num,
                                                      const std::vector<int> &sample_nums);

#endif // SAMPLING_H
