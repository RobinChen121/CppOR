/*
 * Created by Zhen Chen on 2026/9/3.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "../../utils/common.h"
#include "workforce_plan_new.h"

namespace {
// enum 会自动根据顺序分配整数值，默认从 0
// 开始，下面一个是上面一个的值加 1，依次类推
enum class TurnoverType { Stationary, Increasing, Decreasing, Seasonal, TurnoverCount };
const std::vector<std::string> TurnoverTypeNames = {"Stationary", "Increasing", "Decreasing",
                                                    "Seasonal"};

enum class MinWorkerType {
  STA,
  LCY1,
  LCY2,
  SIN1,
  SIN2,
  RAND,
  EMP1,
  EMP2,
  EMP3,
  EMP4,
  MinWorkerCount // 巧妙记录总行数 (10)
};
// 定义对应的行名映射表（用于输出行名）
const std::vector<std::string> MinWorkerTypeNames = {"STA",  "LCY1", "LCY2", "SIN1", "SIN2",
                                                     "RAND", "EMP1", "EMP2", "EMP3", "EMP4"};
} // namespace

int main() {

  const std::vector<std::vector<double>> turnover_rates = {
      {0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4},
      {0.1, 0.1, 0.1, 0.3, 0.3, 0.3, 0.5, 0.5, 0.5, 0.7, 0.7, 0.7},
      {0.7, 0.7, 0.7, 0.5, 0.5, 0.5, 0.3, 0.3, 0.3, 0.1, 0.1, 0.1},
      {0.1, 0.3, 0.5, 0.7, 0.5, 0.3, 0.1, 0.3, 0.5, 0.7, 0.5, 0.3}};

  const std::vector<double> fix_costs = {2000.0, 4000.0, 6000.0};
  const std::vector<double> salaries = {
      1500.0, 2000.0, 3000.0}; // salary should be lower than penalty, other wise no feasible cost
  const std::vector<double> unit_penalties = {3500.0, 4000.0, 5000.0};

  std::vector<std::vector<int>> min_workers = {// STA
                                               {30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
                                               // LCY1
                                               {46, 49, 50, 50, 49, 46, 42, 38, 33, 28, 23, 18},
                                               // LCY2
                                               {11, 14, 18, 23, 28, 33, 38, 42, 46, 49, 50, 49},
                                               // SIN1
                                               {47, 30, 13, 6, 13, 30, 47, 54, 47, 30, 13, 6},
                                               // SIN2
                                               {36, 30, 24, 21, 24, 30, 36, 39, 36, 30, 24, 21},
                                               // RAND
                                               {63, 27, 10, 24, 1, 23, 33, 35, 67, 7, 14, 41},
                                               // EMP1
                                               {2, 6, 18, 56, 32, 59, 54, 30, 34, 44, 19, 35},
                                               // EMP2
                                               {6, 10, 28, 47, 20, 34, 61, 47, 90, 83, 31, 24},
                                               // EMP3
                                               {5, 14, 32, 17, 18, 24, 9, 22, 24, 14, 20, 38},
                                               // EMP4
                                               {6, 22, 8, 34, 54, 27, 27, 62, 35, 66, 8, 27}};

  const std::string file_name =
      "/Users/zhenchen/Library/CloudStorage/OneDrive-BrunelUniversityLondon/"
      "Numerical-tests/workforce/c++/12periods_testing.csv";

  const std::string head = "turnover pattern,fix cost,salary,penalty, min worker, "
                           "SDPtime, SDP value, MIP time, MIP value, optimality gap, line gap, "
                           "MIP-sS time, MIP-sS value, MIP-sS gap\n";
  append_csv_head(file_name, head);

  for (int i = 0; i < static_cast<int>(TurnoverType::TurnoverCount); i++) {
    for (double fix_cost : fix_costs) {
      for (double salary : salaries) {
        for (double penalty : unit_penalties) {
          for (int j = 0; j < static_cast<int>(MinWorkerType::MinWorkerCount); j++) {
            auto problem =
                WorkforcePlanNew(turnover_rates[i], fix_cost, salary, penalty, min_workers[j]);

            // const auto start_time = std::chrono::high_resolution_clock::now();
            // auto [best_value, best_action] = problem.DP1DVector();
            // const auto end_time = std::chrono::high_resolution_clock::now();
            // const std::chrono::duration<double> elapsed_SDP = end_time - start_time;
            // std::cout << "running time of SDP = " << elapsed_SDP.count() << " seconds\n";
            // std::cout << "optimal value = " << best_value << '\n';

            const auto start_time2 = std::chrono::high_resolution_clock::now();
            auto [mip_value, mip_linearization_gap] = problem.solve_mip();
            // const auto end_time2 = std::chrono::high_resolution_clock::now();
            // const std::chrono::duration<double> elapsed_mip = end_time2 - start_time2;
            // std::cout << "running time of MIP = " << elapsed_mip.count() << " seconds\n";
            // std::cout << "value of MIP = " << mip_value << '\n';
            // const double optimality_gap = best_value - mip_value;
            // const double gap_mip = (best_value - mip_value) / best_value * 100;
            // std::cout << "the optimality gap by MIP is: " << std::fixed << std::setprecision(2)
            //           << gap_mip << "%" << std::endl;
            // const double gap_line =
            //     (mip_linearization_gap + mip_value - best_value) / best_value * 100;
            // std::cout << "the linear gap by MIP is: " << std::fixed << std::setprecision(2)
            //           << gap_line << "%" << std::endl;

            // const auto start_time3 = std::chrono::high_resolution_clock::now();
            // auto sS = problem.solve_mipsS();
            // double mip_sS = problem.simulate_sS(problem.getInitialWorkers(), sS);
            // const auto end_time3 = std::chrono::high_resolution_clock::now();
            // const std::chrono::duration<double> elapsed_sS = end_time3 - start_time3;
            // std::cout << "running time of MIP-sS = " << elapsed_sS.count() << " seconds\n";
            // const double sS_gap = best_value - mip_sS;
            // const double gap_sS = sS_gap / best_value * 100;
            // std::cout << "the optimality gap by MIP-sS is: " << std::fixed <<
            // std::setprecision(2)
            //           << gap_sS << "%" << std::endl;
            // const std::chrono::duration<double> elapsed_mipsS = end_time3 - start_time3;

            // appendCSVRowAny(file_name, TurnoverTypeNames[i], fix_cost, salary, penalty,
            //                 MinWorkerTypeNames[j], elapsed_SDP.count(), best_value,
            //                 elapsed_mip.count(), mip_value, optimality_gap,
            //                 mip_linearization_gap, elapsed_mipsS.count(), mip_sS, sS_gap);
            appendCSVRowAny(file_name, TurnoverTypeNames[i], fix_cost, salary, penalty,
                            MinWorkerTypeNames[j], 0, 0, 0, mip_value, 0, mip_linearization_gap, 0,
                            0, 0);
            std::cout << std::string(50, '*') << std::endl;
          }
        }
      }
    }
  }

  return 0;
}