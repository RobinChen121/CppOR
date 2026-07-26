/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 24/06/2026, 22:59
 * Description:
 *
 */

#ifndef CHEN_SOLVER_JS_READ_FILE_H
#define CHEN_SOLVER_JS_READ_FILE_H

#include "model.h"
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

// // 标准 enum 默认通常占 4 字节（int 或 32 bit）。指定为 uint8_t（无符号 8
// // 位整数）后，每个枚举实例仅占用 1 个字节（8 bit）
// enum class ConstraintSense : uint8_t {
//   LE, // Less Than or Equal (<=)
//   EQ, // Equal (=)
//   GE  // Greater Than or Equal (>=)
// };
//
// enum class VarType : uint8_t { Continuous, Integer, Binary };
//
// // 定义一个数据结构存储稀疏向量，用来记录每个非零变量对应的系数
// using SparseVector = std::unordered_map<int, double>;
//
// struct Constraint {
//   SparseVector lhs;
//   double lb;
//   double ub;
//   ConstraintSense sense;
//   double rhs;
// };
//
// struct Variable {
//   int id;
//   std::string name;
//   double lb = 0.0;
//   double ub = INF;
//   VarType type = VarType::Continuous;
// };
//
// struct Model {
//   int obj_sense = 0;
//   std::vector<VarType> variables;
//   std::unordered_map<std::string, int>
//       var_index;          // 依次按照变量从目标函数，约束条件，上下界中出现的顺序编号
//   SparseVector objective; // 这种方式能够节省存储空间
//   std::vector<Constraint> constraints;
//
//   std::vector<SparseVector> lhs;
//   std::vector<double> rhs;
//   std::vector<ConstraintSense> constraint_sense; // <=: 0, =: 1, >=: 2
//   std::vector<VarType> var_type;                 // 0: continuous, 1: integer, 2: binary
//   std::vector<double> lower_bound;
//   std::vector<double> upper_bound;
//   std::vector<bool> free_var;
//
//   // 根据变量名字返回对应的下标
//   int ensureVar(const std::string &name) {
//     if (const auto it = var_index.find(name); it != var_index.end())
//       return it->second;
//     const int index = static_cast<int>(var_names.size());
//     var_index[name] = index;
//     var_names.push_back(name);
//     lower_bound.push_back(0.0);
//     upper_bound.push_back(INF);
//     free_var.push_back(false);
//     var_type.push_back(VarType::Continuous);
//     return index;
//   }
//
//   void addConstraint(const std::unordered_map<int, double> &row, const int sense,
//                      const double value) {
//     lhs.push_back(row);
//     constraint_sense.push_back(sense);
//     rhs.push_back(value);
//   }
//
//   [[nodiscard]] int numVars() const { return static_cast<int>(variables.size()); }
//
//   [[nodiscard]] int numRows() const { return static_cast<int>(lhs.size()); }
//
//   void print();
// };

Model readLP(const std::string &path);
// Model readMPS(const std::string &path);

// Model read(const std::string &path);

#endif // CHEN_SOLVER_JS_READ_FILE_H
