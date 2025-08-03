/*
 * Created by Zhen Chen on 2025/3/8.
 * Email: chen.zhen5526@gmail.com
 * Description: 
 *
 *
 */

#ifndef SIMPLEX_H
#define SIMPLEX_H
#include <vector>

enum class Comparison {
  LessOrEqual,
  Equal,
  GreaterOrEqual
};

enum class VarSign {
  NonNegative,  // ≥ 0
  NonPositive   // ≤ 0
};

class Simplex {
private:
  std::vector<double> obj_coefs;
  bool obj_senses;
  std::vector<std::vector<double>> conLHSs;
  std::vector<double> conRHSs;
  std::vector<Comparison> conComparisons;
  std::vector<VarSign> varSigns;

  std::vector<std::vector<double> > tableau; // 单纯形表
  int rows, cols;
  std::vector<int> basicVars;

    // 找到主列（进入变量）
    int findPivotColumn() const;

    // 找到主行（离开变量）
    int findPivotRow(int pivotCol) const;

    // 行变换
    void pivot(const int pivotRow, int pivotCol);

public:
    explicit Simplex(const std::vector<std::vector<double> > &initialTableau);

    void inputObjCoef() const;

    void solve();

    int isBasicVariable(int col) const;

    void displaySolution() const;

    void initializeBasicVariables();

    void initializeObjective();

    void displayTableau() const;
};

#endif //SIMPLEX_H
