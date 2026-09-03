/*
 * Created by Zhen Chen on 2026/9/3.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */

#include "../../utils/common.h"
#include "workforce_plan_new.h"

// enum 会自动根据顺序分配整数值，默认从 0
// 开始，下面一个是上面一个的值加 1，依次类推
enum class TurnoverType { Stationary, Increasing, Decreasing, Seasonal, Count };
const std::vector<std::string> TurnoverTypeNames = {"Stationary", "Increasing", "Decreasing",
                                                    "Seasonal"};

// 1. 定义行索引的 enum
enum Pattern {
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
  Count // 巧妙记录总行数 (10)
};

// 2. 定义对应的行名映射表（用于输出行名）
const std::vector<std::string> PatternNames = {"STA",  "LCY1", "LCY2", "SIN1", "SIN2",
                                               "RAND", "EMP1", "EMP2", "EMP3", "EMP4"};

int main() {

  const std::vector<std::vector<double>> turnover_rates = {
      {0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4},
      {0.1, 0.1, 0.1, 0.3, 0.3, 0.3, 0.5, 0.5, 0.5, 0.7, 0.7, 0.7},
      {0.7, 0.7, 0.7, 0.5, 0.5, 0.5, 0.3, 0.3, 0.3, 0.1, 0.1, 0.1},
      {0.1, 0.3, 0.5, 0.7, 0.5, 0.3, 0.1, 0.3, 0.5, 0.7, 0.5, 0.3}};

  const std::vector<double> fix_costs = {2000.0, 4000.0, 6000.0};
  const std::vector<double> salaries = {1500.0, 2500.0, 3500.0};
  const std::vector<double> unit_penalties = {2000.0, 3000.0, 4000.0};

  std::vector<std::vector<int>> min_workers = {
    // STA
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
    {6, 22, 8, 34, 54, 27, 27, 62, 35, 66, 8, 27}
  };



  return 0;
}