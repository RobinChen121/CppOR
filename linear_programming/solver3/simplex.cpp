/*
 * Created by Zhen Chen on 2026/6/15.
 * Email: chen.zhen5526@gmail.com
 * Description: the simplex algorithm using compressed sparse column (CSC).
 * Features:
 *   - Standardisation from general LP to Ax = b, x >= 0
 *   - Two-phase primal simplex
 *   - A is stored in CSC
 *   - Basis inverse replaced by BasisFactorization abstraction
 *   - Current concrete implementation: DenseBasisFactorization
 *
 *
 */

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

#include "config.h"
#include "read_file.h"
#include "util.h"

// 一个例子 [[0, 0, 3, 0], [5, 0, 0, 0], [0, 0, 0, 2], [0, 8, 0, 1]]
// 0x_1 + 0x_2 + 3x_3 + 0x_4
// 5x_1 + 0x_2 + 0x_3 + 0x_4
// 0x_1 + 0x_2 + 0x_3 + 2x_4
// 0x_1 + 8x_2 + 0x_3 + 1x_4
// values = {5, 8, 3, 2, 1}
// row_indices = {1, 3, 0, 2, 3}
// col_ptr = {0, 1, 2, 3, 5}, col[i+1]-col[i]为第i列的非零元素个数
struct CSC {
  std::vector<double> values{};   // non zeros values
  std::vector<int> row_indices{}; // row indices for the non-zero values
  // col_ptr 的个数为列数加 1
  // 记录每一列的第一个非零元素在 values 数组中的起始位置（索引）
  std::vector<int> col_ptr{}; // start and end indices in the non-zero values in each column

  CSC() = default; // 空对象方便临时使用
  // nnz = 预计非零元素个数
  // cols = 矩阵列数
  // 工业级时最好估计非零个数，从而提前分配内存
  // array 也可以直接用数值初始化，此时元素都为0
  CSC(const int nnz, const int cols) : values(nnz), row_indices(nnz), col_ptr(cols + 1) {}
};

struct SimplexTableau {
  int phase{};
  int iteration{};
  std::vector<int> basis;
  std::vector<std::vector<double>> coefficients;
  std::vector<double> rhs;
  std::vector<double> reduced_costs;
  double objective_value{};
  int entering_col{-1};
  int leaving_row{-1};
};

/* ============================================================
 * BasisFactorization interface
 * ============================================================ */
// 这个类是一个 抽象接口（abstract interface），它定义了“基矩阵分解器”应该具备哪些能力，
// 但不规定具体怎么实现.
// 后面你可以有不同实现：
// DenseBasisFactorization：用稠密 LU
// EtaBasisFactorization：用 eta 矩阵
// SparseLuBasisFactorization：用稀疏 LU / Forrest–Tomlin
// 它们都遵守这一套接口，但单纯形主循环不用改.
// 工业实现不显式存 B−1，而是维护 B 的某种可逆表示（比如 LU、eta、Forrest–Tomlin 更新），
// 然后通过：FTRAN, BTRAN 来“实现 B^{-1} 乘向量”的效果
class BasisFactorization {
public:
  // ~表示析构函数，生命周期结束时，释放资源
  // 接口基类即带虚函数的类一般要有这个析构函数
  virtual ~BasisFactorization() = default;

  // 纯虚函数 virtual ... = 0：这个函数为一个抽象接口，当前不实现，子类必须实现具体算法
  // 如果非纯虚 virtual ...，基类可以提供一个默认实现，子类可以重写，也可以不重写
  // Build / rebuild factorization from the current basis
  virtual bool factorize(const CSC &A, const std::vector<int> &basis, int m) = 0;

  // FTRAN: solve B x = rhs
  // forward transformation
  // 计算当前基变量 x = B^- * b (相当于解方程 Bx=b),入基方向 d = B^- * a
  [[nodiscard]] virtual std::vector<double>
  forwardTransform(const std::vector<double> &rhs) const = 0;

  // BTRAN: solve B^T x = rhs
  // backward transformation
  // 用于计算对偶信息/reduced cost: c_j - c_B^T * B^- * a
  // 对偶值为 pi^T = c_B^T * B^-
  // pi = (B^-)^T * c_B, 相当于解方程 B^T*pi=c_B
  [[nodiscard]] virtual std::vector<double>
  backwardTransform(const std::vector<double> &rhs) const = 0;

  // Replace a basis column after pivot
  virtual bool replaceColumn(const CSC &A, const std::vector<int> &basis, int leave_row,
                             int entering_col, int m) = 0;
};

/* ============================================================
 * DenseBasisFactorization
 *
 * First implementation:
 *   - Build dense basis matrix B from CSC A and current basis
 *   - LU factorization with partial pivoting
 *   - FTRAN / BTRAN through solve(Bx=rhs) and solve(B^T x=rhs)
 *   - replace_column() simply refactorises from scratch
 * ============================================================ */
// public inheritance：基类的 public 仍是 public，protected 仍是 protected
// protected inheritance：基类的 public 和 protected 都会变成派生类里的 protected
// private inheritance：基类的 public 和 protected 都会变成派生类里的 private
class DenseBasisFactorization : public BasisFactorization {
public:
  DenseBasisFactorization() = default;

  bool factorize(const CSC &A, const std::vector<int> &basis, const int m) override {
    m_ = m;
    basis_ = basis;

    // B_ 现在是一维数组，按 row-major 存储
    // 大小为 m_ * m_
    B_.assign(m_ * m_, 0.0);

    for (int j = 0; j < m_; ++j) {
      const std::vector<double> col = getColumnDense(A, basis_[j], m_);
      for (int i = 0; i < m_; ++i) {
        B(i, j) = col[i]; // 这个B其实是一个函数
      }
    }

    eta_updates_.clear();
    return luFactorize(B_); // LU 分解
  }

  [[nodiscard]] std::vector<double>
  forwardTransform(const std::vector<double> &rhs) const override {
    std::vector<double> x = solveBaseEquations(rhs, false); // solve B0 x = rhs
    for (const EtaUpdate &eta : eta_updates_) {
      applyEtaInverse(eta, x); // 这个应该是有计算规律的
    }
    return x;
  }

  [[nodiscard]] std::vector<double>
  backwardTransform(const std::vector<double> &rhs) const override {
    std::vector<double> x(rhs);
    for (const auto &eta_update : std::ranges::reverse_view(eta_updates_)) {
      applyEtaTransposeInverse(eta_update, x);
    }
    return solveBaseEquations(x, true); // solve B0^T x = rhs
  }

  // 若这个函数的两个参数里有 /* 表示这个输入参数可以不要求输入
  // 每次调用时，basis 可能会变
  bool replaceColumn(const CSC &A, const std::vector<int> &basis, const int leave_row,
                     const int entering_col, const int m) override {
    assert(m == m_);
    const std::vector<double> entering = getColumnDense(A, entering_col, m_);
    std::vector<double> eta_col = forwardTransform(entering);
    if (std::abs(eta_col[leave_row]) <= EPS)
      return false;

    basis_ = basis;
    eta_updates_.push_back({leave_row, std::move(eta_col)});

    // Keep the update chain bounded for numerical stability and solve speed.
    if (static_cast<int>(eta_updates_.size()) >= max_eta_updates_) {
      return factorize(A, basis, m);
    }

    return true;
  }

private:
  struct EtaUpdate {
    int pivot_row{};
    std::vector<double> column;
  };

  int m_{};
  std::vector<int> basis_;
  int max_eta_updates_ =
      64; // 工业级求解器中，这个可能是动态更新的，当 solve time > refactor time 时重新分解

  // 一维 row-major dense matrix
  std::vector<double> B_;
  std::vector<double> LU_;
  std::vector<EtaUpdate> eta_updates_;

  // piv_ 最终保存的是：经过所有 pivoting 之后，当前第 i 行对应原来的哪一行
  std::vector<int> piv_; // row pivoting

  // 行优先下标
  [[nodiscard]] inline int idx(const int i, const int j) const { return i * m_ + j; }

  // 访问 B_
  // 下面两个同名函数，只写 const 函数不能修改 B_，只写非 const 导致类为 const 时调用B出错
  // & 返回底层矩阵元素的可修改引用，这样才能真正修改原始元素值，否则只能返回一个副本了
  inline double &B(const int i, const int j) { return B_[idx(i, j)]; }
  [[nodiscard]] inline const double &B(const int i, const int j) const { return B_[idx(i, j)]; }

  // 访问 LU_
  inline double &LU(const int i, const int j) { return LU_[idx(i, j)]; }
  [[nodiscard]] inline const double &LU(const int i, const int j) const { return LU_[idx(i, j)]; }

  // // get all values for a given column in the dense matrix
  static std::vector<double> getColumnDense(const CSC &A, const int col, const int m) {
    std::vector<double> a(m, 0.0);
    for (int p = A.col_ptr[col]; p < A.col_ptr[col + 1]; ++p) {
      a[A.row_indices[p]] = A.values[p];
    }
    return a;
  }

  // M 也是 row-major 一维矩阵，大小 m_ * m_
  // LU 分解，将一个矩阵分解成一个下三角与上三角矩阵的乘积
  // 对角线以上（含对角线）的位置存 U
  // 对角线以下的位置存 L 的乘子
  // L 的对角线 1 不存
  bool luFactorize(const std::vector<double> &M) {
    LU_ = M;

    piv_.resize(m_);
    for (int i = 0; i < m_; ++i)
      piv_[i] = i;

    // 外层的k是列
    for (int k = 0; k < m_; ++k) {
      int pivot = k;
      double best = std::abs(LU(k, k)); // LU 也是函数

      // 部分主元选取
      // 找每一列的最大值，避免除以 0 或过小主元，提高稳定新
      for (int i = k + 1; i < m_; ++i) {
        if (std::abs(LU(i, k)) > best) {
          best = std::abs(LU(i, k));
          pivot = i;
        }
      }

      if (best <= EPS)
        return false;

      // 交换整行
      if (pivot != k) {
        for (int j = 0; j < m_; ++j) {
          std::swap(LU(pivot, j), LU(k, j));
        }
        std::swap(piv_[pivot], piv_[k]);
      }

      // 消元
      // k 是列
      for (int i = k + 1; i < m_; ++i) {
        // i 是 行
        LU(i, k) /= LU(k, k);              // L 矩阵中的元素，根据公式，刚好就是 L 矩阵元素值
        for (int j = k + 1; j < m_; ++j) { // U 的第一列与原始矩阵相同
          LU(i, j) -= LU(i, k) * LU(k, j); // U 矩阵中的元素，也是公式，包含对角线
        }
      }
    }

    return true;
  }

  static void applyEtaInverse(const EtaUpdate &eta, std::vector<double> &x) {
    const int r = eta.pivot_row;
    const double xr = x[r] / eta.column[r];
    for (int i = 0; i < static_cast<int>(x.size()); ++i) {
      if (i == r)
        continue;
      x[i] -= eta.column[i] * xr;
    }
    x[r] = xr;
  }

  static void applyEtaTransposeInverse(const EtaUpdate &eta, std::vector<double> &x) {
    const int r = eta.pivot_row;
    double xr = x[r];
    for (int i = 0; i < static_cast<int>(x.size()); ++i) {
      if (i == r)
        continue;
      xr -= eta.column[i] * x[i];
    }
    x[r] = xr / eta.column[r];
  }

  [[nodiscard]] std::vector<double> solveBaseEquations(const std::vector<double> &rhs,
                                                       const bool transposed) const {
    // 如果 assert 后面的表达式为 true，程序继续执行；如果为 false，程序会立即终止，并在标准错误输出
    // assert 判断信息 只在 debug模式下运行，release 模式下不运行
    assert(static_cast<int>(rhs.size()) == m_);
    std::vector x(rhs);

    if (!transposed) {
      // Solve Bx = rhs
      // LU分解得到的其实是 PB = LU
      // With row pivoting: P B = L U  =>  L U x = P rhs

      // P 是一个矩阵，存储的行交换信息，在这里用一维数组 piv_ 替代了
      std::vector<double> pb(m_); // pb 就是上面的 PB
      for (int i = 0; i < m_; ++i)
        pb[i] = x[piv_[i]];
      x = pb; // x 初始化为行变换后的 rhs

      // Forward solve Ly = Pb
      // L 的对角线是 1，所以这里不需要除
      // x 先等于一个 y
      for (int i = 0; i < m_; ++i) {
        for (int j = 0; j < i; ++j) {
          x[i] -= LU(i, j) * x[j];
        }
      }

      // Backward solve Ux = y
      for (int i = m_ - 1; i >= 0; --i) {
        for (int j = i + 1; j < m_; ++j) {
          x[i] -= LU(i, j) * x[j];
        }
        x[i] /= LU(i, i);
      }

      return x;
    } else {
      // Solve B^T x = rhs
      // P B = L U  =>  B = P^T L U
      // B^T = U^T L^T P
      //
      // So:
      //   1) solve U^T y = rhs
      //   2) solve L^T z = y
      //   3) x = P^T z = P^T * L^{-T} * U^{-T} * rhs = (U^T*L^T*P)^- * rhs=(B^T)^- * rhs

      // 1) solve U^T y = rhs
      for (int i = 0; i < m_; ++i) {
        for (int j = 0; j < i; ++j) {
          x[i] -= LU(j, i) * x[j];
        }
        x[i] /= LU(i, i);
      }

      // 2) solve L^T z = y
      // L 对角线为 1，所以这里也不需要除
      for (int i = m_ - 1; i >= 0; --i) {
        for (int j = i + 1; j < m_; ++j) {
          x[i] -= LU(j, i) * x[j];
        }
      }

      // 3) x = P^Tz
      std::vector<double> out(m_, 0.0);
      for (int i = 0; i < m_; ++i)
        out[piv_[i]] = x[i];

      return out;
    }
  }
};

class LinearModel {
  // input parameters
  std::vector<double> obj_coe; // objective coefficients
  int obj_sense;               // 0:min, 1: max
  int original_obj_sense;      // keep the user's objective sense for reporting
  std::vector<std::vector<double>> con_lhs;
  std::vector<double> con_rhs;
  std::vector<int> constraint_sense; // 0:<=, 1: >=, 2: =
  std::vector<int> var_sign;         // 0: >=, 1: <=, 2: unsigned

  int enter_rule{}; // 0: Bland, 1: Dantzig, 2: lexicographic

  // middle parameters
  int m;   // number of constraints
  int n0;  // number of original variables
  int n{}; // number of variables after standardizing

  CSC A;
  std::vector<double> c; // objective coefficients after standardizing
  std::vector<double> b; // right hand sides after standardizing

  int num_slack{};
  int num_artificial{};
  // basis[i] = column index of the basic variable in row i
  std::vector<int> basis;
  // column type or var type:
  // 0 = structural / transformed structural
  // 1 = slack
  // 2 = artificial
  std::vector<int> column_type;
  // map_old_to_new[i] = starting column of original variable i
  // if var i is free, then:
  //   x_i = x_i^+ - x_i^-
  //   x_i^+ at map_old_to_new[i], x_i^- at map_old_to_new[i] + 1
  std::vector<int> map_old_to_new;
  // original variable sign for solution recovery
  std::vector<int> original_var_sign;
  std::vector<int> rhs_sign;

  // output
  int solution_status = {
      3}; // 0 optimal, 1 unbounded, 2 infeasible, 3 unsolved, 4 numerical/cycling
  double run_time = {};
  double objective_value = 0.0;
  std::vector<double> primal_solution_standard;
  std::vector<double> primal_solution_original;
  std::vector<double> constraint_dual_values;
  bool save_tableau_history = false;
  std::vector<SimplexTableau> tableau_history;

public:
  LinearModel(const int obj_sense, const std::vector<double> &obj_coe,
              const std::vector<std::vector<double>> &con_lhs, const std::vector<double> &con_rhs,
              const std::vector<int> &constraint_sense, const std::vector<int> &var_sign)
      : obj_coe(obj_coe), obj_sense(obj_sense), con_lhs(con_lhs), con_rhs(con_rhs),
        constraint_sense(constraint_sense), var_sign(var_sign) {
    m = static_cast<int>(con_lhs.size());
    n0 = static_cast<int>(var_sign.size());
    original_var_sign = var_sign;
    original_obj_sense = obj_sense;
  };

  LinearModel(const ParsedLinearProgram &lp) {
    obj_sense = lp.obj_sense;
    n0 = static_cast<int>(lp.objective.size());
    m = static_cast<int>(lp.lhs.size());

    auto values_view = std::views::values(lp.objective);
    // 使用迭代器范围进行赋值
    obj_coe.assign(values_view.begin(), values_view.end());

    original_var_sign = var_sign;
    original_obj_sense = obj_sense;
  };

  [[nodiscard]] std::vector<double> getColumnDense(int col) const;
  [[nodiscard]] int chooseEnteringColumn(const std::vector<double> &phase_c,
                                         const std::vector<bool> &allow_enter,
                                         const std::vector<double> &y) const;
  [[nodiscard]] int chooseLeavingRow(const std::vector<double> &xB, const std::vector<double> &d,
                                     const BasisFactorization &factor) const;
  static std::vector<double> basisInverseRow(const BasisFactorization &factor, int row, int m);
  void standardize();
  bool cleanupArtificialBasis(BasisFactorization &factor);
  bool solvePrimalSimplex();
  bool simplexPhase(const std::vector<double> &phase_c, const std::vector<bool> &allow_enter,
                    BasisFactorization &factor, int max_iter, double &phase_obj, int phase);
  void recordTableau(const std::vector<double> &phase_c, const BasisFactorization &factor,
                     const std::vector<double> &xB, const std::vector<double> &y, double phase_obj,
                     int phase, int iteration, int entering_col, int leaving_row);
  void printSolution() const;
  void print() const;
  void setEnterRule(const int i) { enter_rule = i; };
  void setSaveTableauHistory(const bool value) { save_tableau_history = value; }
  [[nodiscard]] bool getSaveTableauHistory() const { return save_tableau_history; }
  [[nodiscard]] const std::vector<SimplexTableau> &getTableauHistory() const {
    return tableau_history;
  }
  [[nodiscard]] std::vector<double> getConstraintDualValues() const {
    if (solution_status != 0)
      return {};
    return constraint_dual_values;
  }
  [[nodiscard]] bool isInBasis(const int col) const {
    return std::ranges::find(basis, col) != basis.end();
  }
};

// get the column values from the original dense matrix
std::vector<double> LinearModel::getColumnDense(const int col) const {
  std::vector<double> a(m, 0.0);
  for (int p = A.col_ptr[col]; p < A.col_ptr[col + 1]; ++p) {
    a[A.row_indices[p]] = A.values[p];
  }
  return a;
}

int LinearModel::chooseEnteringColumn(const std::vector<double> &phase_c,
                                      const std::vector<bool> &allow_enter,
                                      const std::vector<double> &y) const {
  int enter_col = -1;
  double best_reduced_cost = INF;

  for (int j = 0; j < n; ++j) {
    if (!allow_enter[j] || isInBasis(j))
      continue;

    const std::vector<double> aj = getColumnDense(j);
    // reduced cost = c_j - c_B^T * B^- * a_j
    const double reduced_cost = phase_c[j] - dot(y, aj);

    if (enter_rule == 0 || enter_rule == 2) {
      if (reduced_cost < -EPS) // Bland or lexi rule
        return j;              // 直接返回最小行索引
    } else if (reduced_cost < -EPS && reduced_cost < best_reduced_cost - EPS) {
      enter_col = j;
      best_reduced_cost = reduced_cost;
    }
  }

  return enter_col;
}

// 相当于取 B^- 的指定行
std::vector<double> LinearModel::basisInverseRow(const BasisFactorization &factor, const int row,
                                                 const int m) {
  std::vector<double> unit(m, 0.0);
  unit[row] = 1.0;
  return factor.backwardTransform(unit);
}

int LinearModel::chooseLeavingRow(const std::vector<double> &xB, const std::vector<double> &d,
                                  const BasisFactorization &factor) const {
  int leave_row = -1;
  double min_ratio = INF;
  std::vector<double> best_lex_row;

  for (int i = 0; i < m; ++i) {
    if (d[i] <= EPS)
      continue;

    const double ratio = xB[i] / d[i];
    if (enter_rule == 0) {
      // 相同时取最小行索引
      if (ratio < min_ratio - EPS || (std::abs(ratio - min_ratio) <= EPS &&
                                      (leave_row == -1 || basis[i] < basis[leave_row]))) {
        min_ratio = ratio;
        leave_row = i;
      }
    } else if (enter_rule == 1) {
      if (ratio < min_ratio - EPS) {
        min_ratio = ratio;
        leave_row = i;
      }
    } else {
      const std::vector<double> lex_row = basisInverseRow(factor, i, m);
      bool take = false;

      if (leave_row == -1 || ratio < min_ratio - EPS) {
        take = true;
      } else if (std::abs(ratio - min_ratio) <= EPS) { // 只对 ratio 相等的行采用lexi比较
        bool all_equal = true;
        for (int k = 0; k < m; ++k) {
          const double lhs = lex_row[k] / d[i];
          const double rhs = best_lex_row[k] / d[leave_row];
          if (lhs < rhs - EPS) {
            take = true;
            all_equal = false;
            break;
          }
          if (lhs > rhs + EPS) {
            all_equal = false;
            break;
          }
        }
        if (all_equal && basis[i] < basis[leave_row]) // 全部相等时选择最小列索引
          take = true;
      }

      if (take) {
        min_ratio = ratio;
        leave_row = i;
        best_lex_row = lex_row;
      }
    }
  }

  return leave_row;
}

void LinearModel::standardize() {

  // step 0: convert max → min
  if (obj_sense == 1) {
    for (auto &v : obj_coe)
      v = -v;
    obj_sense = 0;
  }

  // step 1: x <= 0  --->  x = -y, y >= 0
  for (int i = 0; i < n0; ++i) {
    if (var_sign[i] == 1) {
      obj_coe[i] = -obj_coe[i];
      for (int j = 0; j < m; ++j)
        con_lhs[j][i] = -con_lhs[j][i];
      var_sign[i] = 0;
    }
  }

  // step2: RHS normalize to non-negative
  rhs_sign.assign(m, 1);
  // 对于无符号变量，后面用 map_old_to_new 处理，避免使用 insert (复杂度平方级)
  for (int j = 0; j < m; j++) {
    if (con_rhs[j] < 0) { // rhs 没必要用 EPS 了
      rhs_sign[j] = -1;
      con_rhs[j] = -con_rhs[j];
      for (int i = 0; i < n0; i++)
        con_lhs[j][i] = -con_lhs[j][i];

      if (constraint_sense[j] != 2)
        constraint_sense[j] = 1 - constraint_sense[j];
    }
  }

  // step 3: compute final variable count
  n = n0;
  // free variable split: +1 extra variable each
  for (int i = 0; i < n0; ++i) {
    if (var_sign[i] == 2)
      ++n;
  }
  // add slack/artificial
  for (int j = 0; j < m; ++j) {
    if (constraint_sense[j] == 0) { // <= : + slack
      ++n;
      ++num_slack;
    } else if (constraint_sense[j] == 1) { // >= : + slack + artificial
      n += 2;
      ++num_slack;
      ++num_artificial;
    } else { // = : + artificial
      ++n;
      ++num_artificial;
    }
  }

  // step 4:  map original variables to standardized columns
  // map_old_to_new[i] 表示原始第 i 个变量，在“标准化后变量空间”中的起始列索引
  map_old_to_new.resize(n0);
  int col_num = 0;
  for (int i = 0; i < n0; i++) {
    map_old_to_new[i] = col_num;
    col_num++;
    if (var_sign[i] == 2)
      col_num++; // extra column for x_i^-
  }

  const int base_var_end = col_num;

  // step 5: allocate c / b / basis / column_type
  c.assign(n, 0.0);
  b = con_rhs;
  basis.assign(m, -1);
  column_type.assign(n, 0);

  // fill in objective values
  for (int i = 0; i < n0; ++i) {
    const int c1 = map_old_to_new[i];
    c[c1] = obj_coe[i];
    column_type[c1] = 0;

    if (var_sign[i] == 2) {
      const int c2 = c1 + 1;
      c[c2] = -obj_coe[i];
      column_type[c2] = 0;
    }
  }

  // step 6: count nnz (number of non zeros)
  std::vector col_nnz(n, 0); // 每一列非零元素的个数
  // original vars (structural columns)
  for (int row = 0; row < m; row++) {
    for (int i = 0; i < n0; i++) {
      if (const double v = con_lhs[row][i]; !is_zero(v)) {
        col_nnz[map_old_to_new[i]]++;
        if (var_sign[i] == 2)
          col_nnz[map_old_to_new[i] + 1]++;
      }
    }
  }

  // slack/artificial columns and initial basis
  int cur = base_var_end;
  for (int row = 0; row < m; ++row) {
    if (constraint_sense[row] == 0) { // <= : + slack
      ++col_nnz[cur];
      column_type[cur] = 1;
      basis[row] = cur; // slack basic
      ++cur;
    } else if (constraint_sense[row] == 1) { // >= : -slack + artificial
      ++col_nnz[cur];
      column_type[cur] = 1; // slack
      ++cur;

      ++col_nnz[cur];
      column_type[cur] = 2; // artificial
      basis[row] = cur;     // artificial basic
      ++cur;
    } else { // = : + artificial
      ++col_nnz[cur];
      column_type[cur] = 2;
      basis[row] = cur;
      ++cur;
    }
  }
  assert(cur == n);

  // step 7: build CSC
  // init CSC
  int nnz = 0;
  for (const int x : col_nnz)
    nnz += x;

  A = CSC(nnz, n);
  A.col_ptr[0] = 0;
  for (int i = 0; i < n; i++)
    A.col_ptr[i + 1] = A.col_ptr[i] + col_nnz[i];
  // offset[j] 记录的是：当前正在处理 column j 时，已经扫描到的位置（cursor / 游标）
  // offset[j] = next free slot in column j
  std::vector<int> offset = A.col_ptr;

  // fill CSC
  // original vars (structural columns)
  for (int row = 0; row < m; row++) {
    for (int i = 0; i < n0; i++) {
      const double v = con_lhs[row][i];
      if (is_zero(v))
        continue;

      const int c1 = map_old_to_new[i]; // 原始第i个变量在标准化矩阵中的起始列索引
      const int p = offset[c1]++;       // 等价于
      // int p = offset[c1];
      // offset[c1] = offset[c1] + 1; // 必须用++，因为同一列可能有多个非0元素

      A.values[p] = v;
      A.row_indices[p] = row;

      if (var_sign[i] == 2) {
        const int c2 = c1 + 1;
        const int p2 = offset[c2]++;
        A.values[p2] = -v;
        A.row_indices[p2] = row;
      }
    }
  }

  // 一个例子 [[0, 0, 3, 0], [5, 0, 0, 0], [0, 0, 0, 2], [0, 8, 0, 1]]
  // values = {5, 8, 3, 2, 1}
  // row_indices = {1, 3, 0, 2, 3}
  // col_start_end = {0, 1, 2, 3, 5}, col[i+1]-col[i]为第i列的非零元素个数
  // fill slack/artificial columns
  cur = base_var_end;
  for (int row = 0; row < m; ++row) {
    if (constraint_sense[row] == 0) {
      // <= : +slack
      // ++ 是为了保证 offset[j] == col_ptr[j+1]
      const int p = offset[cur]++;
      A.values[p] = 1.0;
      A.row_indices[p] = row;
      ++cur;
    } else if (constraint_sense[row] == 1) {
      // >= : -slack + artificial
      const int p1 = offset[cur]++;
      A.values[p1] = -1.0;
      A.row_indices[p1] = row;
      ++cur;

      const int p2 = offset[cur]++;
      A.values[p2] = 1.0;
      A.row_indices[p2] = row;
      ++cur;
    } else {
      // = : +artificial
      const int p = offset[cur]++;
      A.values[p] = 1.0;
      A.row_indices[p] = row;
      ++cur;
    }
  }
  // 对于变量可以直接做基变量的情况
  // 但工业实现不会单独特判这一种情况
  // 标准做法是统一加 artificial，保证鲁棒性
  // 真正优化是在 presolve 阶段做更一般的结构消除
}

/* ============================================================
 * Try to remove artificial variables from the basis after Phase I
 * ============================================================ */
// 第一阶段结束后，基变量中仍然可能有人工变量，需要替换出去
bool LinearModel::cleanupArtificialBasis(BasisFactorization &factor) {
  bool changed = true;

  while (changed) {
    changed = false;

    const std::vector<double> xB = factor.forwardTransform(b);

    for (int row = 0; row < m; ++row) {
      // ReSharper disable once CppTooWideScopeInitStatement
      const int basic_col = basis[row];
      if (column_type[basic_col] != 2)
        continue; // not artificial

      // artificial variable positive after Phase I -> infeasible
      if (xB[row] > EPS) {
        return false;
      }

      // try to pivot it out using a non-artificial nonbasic column
      for (int j = 0; j < n; ++j) {
        if (column_type[j] == 2)
          continue; // don't pivot in artificial
        if (isInBasis(j))
          continue;

        const std::vector<double> aj = getColumnDense(j);
        const std::vector<double> d = factor.forwardTransform(aj);
        if (std::abs(d[row]) > EPS) { // 找到一个就开始换
          basis[row] = j;
          // basis 变了
          if (!factor.replaceColumn(A, basis, row, j, m)) {
            return false;
          }
          changed = true;
          break;
        }
      }

      // if changed is false, remove one row and column, to do

      if (changed)
        break; // restart scanning
    }
  }

  return true;
}

bool LinearModel::solvePrimalSimplex() {
  const auto start_time = std::chrono::high_resolution_clock::now();

  tableau_history.clear();
  constraint_dual_values.clear();
  primal_solution_standard.clear();
  primal_solution_original.clear();

  standardize();

  // Plug in current basis-factorisation implementation
  // 这行代码创建了一个 Dense LU 基矩阵分解器对象
  // 用统一接口 BasisFactorization 来持有
  // 使得在不改 solver 代码的情况下切换算法
  // unique_prt 是一个智能指针，只允许一个人拥有这个对象
  // 自动释放内存（避免 memory leak）
  // std::make_unique<DenseBasisFactorization>() 等价于 std::make_unique<SparseFactorization>()
  // 但更安全，因为能够自动释放内存
  // 这一行等价于 BasisFactorization* factor = new DenseBasisFactorization()，但更安全
  // 不用指针的话，接口函数中的虚函数会失效，子类方法直接消失
  // ReSharper disable once CppTooWideScopeInitStatement
  const std::unique_ptr<BasisFactorization> factor = std::make_unique<DenseBasisFactorization>();

  if (!factor->factorize(A, basis, m)) {
    solution_status = 4;
    return false;
  }

  // two stage
  // ---------------- Phase I ----------------
  std::vector<double> c_phase1(n, 0.0);
  for (int j = 0; j < n; ++j) {
    if (column_type[j] == 2) {
      c_phase1[j] = 1.0;
    }
  }

  const std::vector allow_enter_phase1(n, true);
  double phase1_obj = 0.0;
  if (!simplexPhase(c_phase1, allow_enter_phase1, *factor, MAX_ITER, phase1_obj, 1)) {
    solution_status = 4;
    return false;
  }

  if (phase1_obj > EPS) {
    solution_status = 2; // infeasible
    return false;
  }

  if (!cleanupArtificialBasis(*factor)) {
    solution_status = 2;
    return false;
  }

  // ---------------- Phase II ----------------
  std::vector allow_enter_phase2(n, true);
  for (int j = 0; j < n; ++j) {
    if (column_type[j] == 2) {
      allow_enter_phase2[j] = false; // artificial variables cannot enter in phase II
    }
  }

  // ReSharper disable once CppTooWideScopeInitStatement
  double phase2_obj = 0.0;
  if (!simplexPhase(c, allow_enter_phase2, *factor, MAX_ITER, phase2_obj, 2)) {
    // if (solution_status == 3) solution_status = 4;
    return false;
  }

  // Optimal
  solution_status = 0;

  // Recover standardized primal solution
  primal_solution_standard.assign(n, 0.0);
  // 之前结果过程中正确的 basis 已经找到了
  const std::vector<double> xB = factor->forwardTransform(b);

  for (int i = 0; i < m; ++i) {
    // ReSharper disable once CppTooWideScopeInitStatement
    const int basic_col = basis[i];
    if (basic_col >= 0 && basic_col < n) {
      primal_solution_standard[basic_col] = xB[i];
    }
  }

  // Recover original variables
  primal_solution_original.assign(n0, 0.0);

  for (int i = 0; i < n0; ++i) {
    const int c1 = map_old_to_new[i];

    if (original_var_sign[i] == 0) {
      // original x >= 0
      primal_solution_original[i] = primal_solution_standard[c1];
    } else if (original_var_sign[i] == 1) {
      // original x <= 0, standardised by x = -y
      primal_solution_original[i] = -primal_solution_standard[c1];
    } else {
      // free variable x = x^+ - x^-
      primal_solution_original[i] = primal_solution_standard[c1] - primal_solution_standard[c1 + 1];
    }
  }

  objective_value = original_obj_sense == 1 ? -phase2_obj : phase2_obj;

  std::vector<double> cB(m, 0.0);
  for (int i = 0; i < m; ++i) {
    cB[i] = c[basis[i]];
  }
  const std::vector<double> dual_min_standard = factor->backwardTransform(cB);
  const double objective_sign = original_obj_sense == 1 ? -1.0 : 1.0;
  constraint_dual_values.assign(m, 0.0);
  for (int i = 0; i < m; ++i) {
    constraint_dual_values[i] = objective_sign * rhs_sign[i] * dual_min_standard[i];
  }

  const auto end_time = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> diff = end_time - start_time;
  run_time = diff.count();
  return true;
}

/* ============================================================
 * One simplex phase
 * ============================================================ */
// phase_c: objetive coefficients in this phase.
bool LinearModel::simplexPhase(const std::vector<double> &phase_c,
                               const std::vector<bool> &allow_enter, BasisFactorization &factor,
                               const int max_iter, double &phase_obj, const int phase) {
  const int max_iterations = max_iter;

  for (int iter = 0; iter < max_iterations; ++iter) {
    // x_B = B^{-1} b  --->  ftran(b)
    const std::vector<double> xB = factor.forwardTransform(b);

    // c_B
    std::vector<double> cB(m, 0.0);
    for (int i = 0; i < m; ++i) {
      cB[i] = phase_c[basis[i]];
    }

    // 对偶值 y^T = c_B^T B^{-1}
    // 对偶值为 pi^T = c_B^T * B^-， pi 就是 y
    // equivalently solve B^T y = c_B
    const std::vector<double> y = factor.backwardTransform(cB);

    // current phase objective
    phase_obj = dot(cB, xB);

    const int enter_col = chooseEnteringColumn(phase_c, allow_enter, y);

    // optimal for this phase
    if (enter_col == -1) {
      recordTableau(phase_c, factor, xB, y, phase_obj, phase, iter, enter_col, -1);
      return true;
    }

    // d = B^{-1} a_j  --->  ftran(a_j)
    const std::vector<double> aj = getColumnDense(enter_col);
    const std::vector<double> d = factor.forwardTransform(aj);

    const int leave_row = chooseLeavingRow(xB, d, factor);

    recordTableau(phase_c, factor, xB, y, phase_obj, phase, iter, enter_col, leave_row);

    if (leave_row == -1) {
      solution_status = 1; // unbounded
      return false;
    }

    // update basis
    basis[leave_row] = enter_col;
    if (!factor.replaceColumn(A, basis, leave_row, enter_col, m)) {
      solution_status = 4;
      return false;
    }
  }

  solution_status = 4; // iteration limit / cycling
  return false;
}

void LinearModel::recordTableau(const std::vector<double> &phase_c,
                                const BasisFactorization &factor, const std::vector<double> &xB,
                                const std::vector<double> &y, const double phase_obj,
                                const int phase, const int iteration, const int entering_col,
                                const int leaving_row) {
  if (!save_tableau_history)
    return;

  SimplexTableau tableau;
  tableau.phase = phase;
  tableau.iteration = iteration;
  tableau.basis = basis;
  tableau.rhs = xB;
  tableau.objective_value = phase_obj;
  tableau.entering_col = entering_col;
  tableau.leaving_row = leaving_row;

  tableau.coefficients.assign(m, std::vector<double>(n, 0.0));
  tableau.reduced_costs.assign(n, 0.0);
  for (int col = 0; col < n; ++col) {
    const std::vector<double> transformed_col = factor.forwardTransform(getColumnDense(col));
    for (int row = 0; row < m; ++row) {
      tableau.coefficients[row][col] = transformed_col[row];
    }

    tableau.reduced_costs[col] = phase_c[col] - dot(y, getColumnDense(col));
  }

  tableau_history.push_back(std::move(tableau));
}

void LinearModel::printSolution() const {
  std::cout << "Status: ";
  switch (solution_status) {
  case 0:
    std::cout << "Optimal";
    break;
  case 1:
    std::cout << "Unbounded";
    break;
  case 2:
    std::cout << "Infeasible";
    break;
  case 4:
    std::cout << "Cycling/Numerical issue";
    break;
  default:
    std::cout << "Unsolved";
    break;
  }
  std::cout << "\n";

  if (solution_status == 0) {
    std::cout << "Objective value = " << std::setprecision(12) << objective_value << "\n";
    std::cout << "Primal solution (original vars):\n";
    for (size_t i = 0; i < primal_solution_original.size(); ++i) {
      std::cout << "x[" << i << "] = " << primal_solution_original[i] << "\n";
    }
    std::cout << "running tim is " << run_time << " s" << std::endl;
  }
}

void LinearModel::print() const {
  const auto print_term = [&](const double coef, const std::string &name, bool &printed) {
    if (is_zero(coef) && n == 0)
      return;

    if (printed) {
      std::cout << (coef < 0.0 ? " - " : " + ");
    } else if (coef < 0.0) {
      std::cout << "-";
    }

    if (const double abs_coef = std::abs(coef); std::abs(abs_coef - 1.0) > EPS)
      std::cout << abs_coef << "*";
    std::cout << name;
    printed = true;
  };

  // 这个匿名函数里面的第一个 & 使得它可以访问匿名函数外面的函数或变量
  const auto print_expr = [&](const std::vector<double> &coef,
                              const std::vector<std::string> &names) {
    bool printed = false;
    for (size_t i = 0; i < coef.size(); ++i)
      print_term(coef[i], names[i], printed);
    if (!printed)
      std::cout << "0";
  };

  const auto sense_text = [](const int sense) {
    if (sense == 0)
      return " <= ";
    if (sense == 1)
      return " >= ";
    return " = ";
  };

  if (n == 0) {
    std::vector<std::string> names(n0);
    for (int i = 0; i < n0; ++i)
      names[i] = "x_" + std::to_string(i + 1);

    std::cout << "Original model\n";
    std::cout << (original_obj_sense == 0 ? "min  " : "max  ");
    print_expr(obj_coe, names);
    std::cout << "\n";
    std::cout << "s.t.\n";

    for (int row = 0; row < m; ++row) {
      std::cout << "  ";
      print_expr(con_lhs[row], names);
      std::cout << sense_text(constraint_sense[row]) << con_rhs[row] << "\n";
    }

    std::cout << "bounds\n";
    for (int i = 0; i < n0; ++i) {
      std::cout << "  " << names[i];
      if (var_sign[i] == 0)
        std::cout << " >= 0";
      else if (var_sign[i] == 1)
        std::cout << " <= 0";
      else
        std::cout << " free";
      std::cout << "\n";
    }
    return;
  }

  // if n > 0, meaning having been standardized, it will run the following
  std::vector<std::string> names(n);
  for (int old_col = 0; old_col < n0; ++old_col) {
    const int col = map_old_to_new[old_col];
    if (original_var_sign[old_col] == 2) {
      names[col] = "x_" + std::to_string(old_col + 1) + "_plus";
      names[col + 1] = "x_" + std::to_string(old_col + 1) + "_minus";
    } else {
      names[col] = "x_" + std::to_string(old_col + 1);
    }
  }

  int slack_count = 0;
  int artificial_count = 0;
  for (int col = 0; col < n; ++col) {
    if (!names[col].empty())
      continue;
    if (column_type[col] == 1) {
      names[col] = "s_" + std::to_string(++slack_count);
    } else if (column_type[col] == 2) {
      names[col] = "a_" + std::to_string(++artificial_count);
    } else {
      names[col] = "x_std_" + std::to_string(col + 1);
    }
  }

  std::cout << "********************************" << std::endl;
  std::cout << "Standardized model\n";
  std::cout << "min  ";
  print_expr(c, names);
  std::cout << "\n";
  std::cout << "s.t.\n";

  for (int row = 0; row < m; ++row) {
    std::vector<double> row_coef(n, 0.0);
    for (int col = 0; col < n; ++col) {
      for (int p = A.col_ptr[col]; p < A.col_ptr[col + 1]; ++p) {
        if (A.row_indices[p] == row) {
          row_coef[col] = A.values[p];
          break;
        }
      }
    }

    std::cout << "  ";
    print_expr(row_coef, names);
    std::cout << " = " << b[row] << "\n";
  }

  std::cout << "bounds\n";
  for (const std::string &name : names)
    std::cout << "  " << name << " >= 0\n";
  std::cout << std::endl;
}

int main() {
  // constexpr int obj_sense = 0; // min
  // const std::vector<double> obj = {-3.0, -2.0};
  // const std::vector<std::vector<double>> lhs = {{1.0, 1.0}, {1.0, 0.0}, {0.0, 1.0}};
  // const std::vector<double> rhs = {4.0, 2.0, 3.0};
  // const std::vector<int> con_sense = {
  //     0, 0, 0 // <=, <=, <=
  // };
  // const std::vector<int> var_sign = {
  //     0, 0 // x1>=0, x2>=0
  // };

  // constexpr int obj_sense = 1;
  // const std::vector obj = {2.0, 3.0};
  // const std::vector<std::vector<double>> lhs = {{2, 1}, {1, 2}};
  // const std::vector rhs = {4.0, 5.0};
  // const std::vector con_sense = {0, 0};
  // const std::vector var_sign = {0, 0};
  //
  // 这个可以在 presolve 中解决
  // constexpr int obj_sense = 1; // 0: min, 1: max, basic var already
  // const std::vector obj = {3.0, 5.0, 0.0, 0.0, 0.0};
  // const std::vector<std::vector<double>> lhs = {
  //     {1.0, 0.0, 1.0, 0, 0},
  //     {0, 2.0, 0, 1.0, 0.0},
  //     {3, 2.0, 0, 0.0, 1.0},
  // };
  // const std::vector rhs = {4.0, 12.0, 18.0};
  // const std::vector con_sense = {2, 2, 2};      // 0:<=, 1: >=, 2: =
  // const std::vector var_sign = {0, 0, 0, 0, 0}; // 0: >=, 1: <=, 2: unsigned

  // constexpr int obj_sense = 0;
  // const std::vector obj = {50.0, 20.0, 30.0, 80.0}; // two-stage
  // const std::vector<std::vector<double>> lhs = {
  //     {400, 200, 100, 500}, {3, 2, 0, 0}, {2, 2, 4, 4}, {2, 4, 1, 5}};
  // const std::vector rhs = {500.0, 6.0, 10.0, 8.0};
  // const std::vector con_sense = {1, 1, 1, 1};
  // const std::vector var_sign = {0, 0, 0, 0};

  // constexpr int obj_sense = 1; // two-stage
  // const std::vector obj = {1.0, 5.0, 3.0};
  // const std::vector<std::vector<double>> lhs = {
  //     {1, 2, 1},
  //     {2, -1, 0},
  // };
  // const std::vector rhs = {3.0, 4.0};
  // const std::vector con_sense = {2, 2};
  // const std::vector var_sign = {0, 0, 0};

  constexpr int obj_sense = 1; // cycling
  const std::vector<double> obj = {0.75, -20, 0.5, -6};
  const std::vector<std::vector<double>> lhs = {
      {0.25, -8, -1, 9}, {0.5, -12, -0.5, 3}, {0, 0, 1, 0}};
  const std::vector rhs = {0.0, 0.0, 1.0};
  const std::vector con_sense = {0, 0, 0};
  const std::vector var_sign = {0, 0, 0, 0};

  // // ReSharper disable once CppTooWideScopeInitStatement
  LinearModel model(obj_sense, obj, lhs, rhs, con_sense, var_sign);
  model.print();
  model.solvePrimalSimplex();
  model.print();
  model.printSolution();

  return 0;
}
