/*
 * Created by Zhen Chen on 2026/9/3.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "../../utils/common.h"
#include "workforce_plan.h"
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

  std::vector<std::vector<int>> min_workers = {
      {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100},
      {162, 166, 166, 162, 153, 141, 126, 110, 93, 77, 61, 48},
      {36, 48, 61, 77, 93, 110, 126, 141, 153, 162, 166, 166},
      {100, 43, 20, 43, 100, 157, 180, 157, 100, 43, 20, 43},
      {100, 79, 70, 79, 100, 121, 130, 121, 100, 79, 70, 79},
      {91, 33, 79, 2, 76, 109, 115, 224, 22, 48, 136, 13},
      {51, 152, 467, 268, 489, 446, 248, 281, 363, 155, 293, 220},
      {81, 236, 394, 164, 287, 508, 391, 754, 694, 261, 195, 320},
      {116, 264, 144, 146, 198, 74, 183, 204, 114, 165, 318, 119},
      {188, 64, 279, 453, 224, 223, 517, 291, 547, 646, 224, 215}};

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
                WorkforcePlan(turnover_rates[i], fix_cost, salary, penalty, min_workers[j]);

            const auto start_time = std::chrono::high_resolution_clock::now();
            // auto [best_value, best_action] = problem.DP1DVector();
            auto [best_value, best_action] = problem.solve(WorkerState{1, 0});
            const auto end_time = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double> elapsed_SDP = end_time - start_time;
            std::cout << "running time of SDP = " << elapsed_SDP.count() << " seconds\n";
            std::cout << "optimal value = " << best_value << '\n';

            // problem.mipPiecewisePrecompute();
            // const auto start_time2 = std::chrono::high_resolution_clock::now();
            // auto [mip_value, mip_linearization_gap] = problem.solveMIP();
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
            //
            // const auto start_time3 = std::chrono::high_resolution_clock::now();
            // auto sS = problem.solveMIPsS();
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
                            MinWorkerTypeNames[j], elapsed_SDP.count(), best_value, 0, 0, 0, 0, 0,
                            0, 0);
            std::cout << std::string(50, '*') << std::endl;
          }
        }
      }
    }
  }

  return 0;
}