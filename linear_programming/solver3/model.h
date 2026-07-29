/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 25/07/2026, 22:07
 * Description:
 *
 */

#ifndef CHEN_SOLVER_JS_MODEL_H
#define CHEN_SOLVER_JS_MODEL_H

#include "config.h"
#include <map>
#include <string>
#include <vector>

// 更加明确这些整型占用的字节数
// ChenInt is for indexing
using ChenInt = int32_t;
using VarId = uint64_t;
using ConId = uint64_t;

// 标准 enum 默认通常占 4 字节（int 或 32 bit）。指定为 uint8_t（无符号 8
// 位整数）后，每个枚举实例仅占用 1 个字节（8 bit）
enum class ObjSense : uint8_t {
  Minimize, // Minimize the objective function
  Maximize,
};

enum class VarType : uint8_t { Continuous, Integer, Binary };

struct Variable {
  VarId id;
  std::string name;
  double lb = 0.0;
  double ub = INF;
  VarType var_type = VarType::Continuous;
};

struct LinearTerm {
  ChenInt col; // column index
  double coef;
};

struct Constraint {
  ConId id{};
  std::string name;
  std::vector<LinearTerm> lhs;
  double lb{};
  double ub{};

  Constraint() = default;
  // 预分配约束条件里面的非零项数目，在模型规模比较大时有必要
  explicit Constraint(const ChenInt num_reserve) { lhs.reserve(num_reserve); };
};

// 给模型添加一个状态
enum class ModelStatus : uint8_t {
  Empty,
  Modified,
  Compiled,
};

// user model
struct Model {
  std::string model_name;
  ModelStatus status = ModelStatus::Empty;
  ObjSense obj_sense = ObjSense::Minimize;
  std::vector<Variable> variables;
  // sparse objective coefficients for each column index
  std::map<ChenInt, double> objective_coef;
  std::vector<Constraint> constraints;

  // map from var name to its variable index
  std::map<std::string, ChenInt> name_to_varIndex;
  // map from constraint name to its constraint index
  std::map<std::string, ChenInt> name_to_conIndex;

  VarId next_var_id = 0;
  ConId next_con_id = 0;

  void addVariable(const std::string &name, double lb = 0.0, double ub = INF,
                   VarType type = VarType::Continuous);
  void addConstraint(const std::string &name, const std::vector<LinearTerm> &terms, double lb,
                     double ub);
  // 检查模型是否有效
  [[nodiscard]] bool valid() const;
  [[nodiscard]] ChenInt nameToVarIndex(const std::string &name) const;
  ChenInt findOrCreateVariable(const std::string &name);

  void print() const;
  void printLinearExpression(const std::vector<LinearTerm> &terms) const;
  void printObjective() const;
  void printConstraints() const;
  void printBounds() const;
  void printIntegers() const;
  void printBinaries() const;
};

// 一个例子 [[0, 0, 3, 0], [5, 0, 0, 0], [0, 0, 0, 2], [0, 8, 0, 1]]
// 0x_1 + 0x_2 + 3x_3 + 0x_4
// 5x_1 + 0x_2 + 0x_3 + 0x_4
// 0x_1 + 0x_2 + 0x_3 + 2x_4
// 0x_1 + 8x_2 + 0x_3 + 1x_4
// values = {5, 8, 3, 2, 1}
// row_indices = {1, 3, 0, 2, 3}
// col_ptr = {0, 1, 2, 3, 5}, col[i+1]-col[i] equals the number of non-zeros in column i
struct CSC {
  std::vector<double> values{};       // non zeros values
  std::vector<ChenInt> row_indices{}; // row indices for the non-zero values
  // col_ptr 的个数为列数加 1
  // 记录每一列的第一个非零元素在 values 数组中的起始位置（索引）
  std::vector<ChenInt> col_ptr{}; // start and end indices in the non-zero values in each column

  CSC() = default; // 空对象方便临时使用
  // num_non_zero = 预计非零元素个数
  // num_col = 矩阵列数
  // 工业级时最好估计非零个数，从而提前分配内存
  // array 也可以直接用数值初始化，此时元素都为 0
  CSC(const ChenInt num_non_zero, const ChenInt num_col)
      : values(num_non_zero), row_indices(num_non_zero), col_ptr(num_col + 1) {}
};

struct SolverModel {
  ObjSense obj_sense = ObjSense::Minimize;
  std::vector<double> objective_coef;
  double obj_offset = 0.0;
  CSC A_matrix;
  std::vector<double> row_ub;
  std::vector<double> row_lb;

  std::vector<double> col_ub;
  std::vector<double> col_lb;
  std::vector<VarType> var_type;

  ChenInt num_row{};
  ChenInt num_col{};
  std::vector<VarId> col_ids;
  std::vector<ConId> row_ids;
};

SolverModel compile(const Model &model);

#endif // CHEN_SOLVER_JS_MODEL_H
