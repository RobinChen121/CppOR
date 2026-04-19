/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 11/04/2026, 11:18
 * Description:
 *
 */

#ifndef CHEN_SOLVER_JS_STATISTIC_PREDICT_H
#define CHEN_SOLVER_JS_STATISTIC_PREDICT_H

#include <vector>

class Predictor {
  std::vector<double> data;

public:
  explicit Predictor(const std::vector<double> &data) : data(data) {};

  std::vector<double> singleExponentialSmooth(double alpha, int forecast_step) const;
};

#endif // CHEN_SOLVER_JS_STATISTIC_PREDICT_H
