/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 2025/7/22, 20:31
 * Description: 
 * 
 */
#include <iostream>
#include <cmath>
#include <iomanip>

// Standard normal CDF via erfc
double norm_cdf(double x) {
  return 0.5 * std::erfc(-x / std::sqrt(2));
}

// Tail probability P(Z >= x)
double tail_prob(double x) {
  return 1.0 - norm_cdf(x);
}

int main() {
  const double step = 0.1;
  const double max_val = 50.0;
  const double tolerance = 1e-2;
  bool found_counterexample = false;

  std::cout << std::fixed << std::setprecision(10);

// #pragma omp parallel for
  for (double j = step; j <= max_val; j += step) {
    for (double k = step; k <= max_val; k += step) {
      double p_j = tail_prob(j);
      double p_k = tail_prob(k);
      double p_jk = 0.5*tail_prob(j + k);

      if (p_jk > p_j * p_k + tolerance) {
        found_counterexample = true;
        std::cout << "Counterexample found:\n";
        std::cout << "j = " << j << ", k = " << k << std::endl;
        std::cout << "P(Z >= j + k)     = " << p_jk << std::endl;
        std::cout << "P(Z >= j)*P(Z >= k) = " << p_j * p_k << std::endl;
        std::cout << "Difference         = " << (p_jk - p_j * p_k) << "\n\n";
      }
    }
  }

  if (!found_counterexample) {
    std::cout << "No counterexample found\n";
    std::cout << "Submultiplicativity appears to hold for standard normal tail.\n";
  }

  return 0;
}
