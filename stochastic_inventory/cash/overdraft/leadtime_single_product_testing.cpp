/*
 * Created by Zhen Chen on 2025/4/6.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */
#include "../../../utils/fileOperations.h"
#include "leadtime_single_product.h"
#include <vector>

int main() {
  std::vector<std::vector<double>> demands_all = {
      {15, 15, 15, 15}, {25.0, 20.0, 10.0, 7.0}, {7, 12, 17, 23}, {24, 15, 5, 20}, {5, 11, 22, 10},
      {31, 14, 25, 12}, {40, 23, 8, 30},         {7, 30, 15, 12}, {17, 6, 31, 22}, {9, 17, 35, 10}};

  std::vector<double> overdraft_interests = {0.2, 0.15, 0.1, 0.05, 0.0};
  std::vector<double> overhead_costs_all = {25.0, 50.0};
  std::vector<double> prices_all = {5, 10};

  int sampleNum = 10;
  int forwardNum = 30;
  int iterNum = 50;

  const std::string file_name =
      "/Users/zhenchen/Library/CloudStorage/OneDrive-BrunelUniversityLondon/"
      "Numerical-tests/overdraft/c++/sdp_testing.csv";
  const std::string head = "demand pattern, interest rate, overhead, price, final value, time, Q\n";
  appendHeadToCSV(file_name, head);

  for (int i = 0; i < demands_all.size(); i++) {
    for (int m = 0; m < overdraft_interests.size(); m++) {
      for (int j = 0; j < overhead_costs_all.size(); j++) {
        for (int k = 0; k < prices_all.size(); k++) {
          const auto &demands = demands_all[i];
          const double overhead = overhead_costs_all[j];
          const double price = prices_all[k];
          double interest = overdraft_interests[m];

          auto problem = OverdraftLeadtimeSingleProduct(demands, interest, overhead, price);
          const auto start_time = std::chrono::high_resolution_clock::now();
          const double final_value = problem.solve()[0];
          const auto end_time = std::chrono::high_resolution_clock::now();
          const std::chrono::duration<double> time = end_time - start_time;
          const double Q = problem.solve()[1];
          std::vector<double> arr = {static_cast<double>(i), interest, overhead, price, final_value,
                                     time.count(),           Q};
          appendRowToCSV(file_name, arr);
          std::cout << "**************************************************" << std::endl;
          std::cout << "running time is " << time.count() << 's' << std::endl;
          std::cout << "Final expected cash increment is " << final_value << std::endl;
          std::cout << "Optimal Q in the first period is " << problem.solve()[1] << std::endl;
        }
      }
    }
  }

  return 0;
}