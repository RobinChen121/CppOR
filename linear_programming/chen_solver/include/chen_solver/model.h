/*
 * Created by Zhen Chen on 2026/8/19.
 * Email: chen.zhen5526@gmail.com
 * Description: linear model for the solver
 *
 *
 */

#ifndef CPPOR_MODEL_H
#define CPPOR_MODEL_H

#include <string>
#include

class ModelLP {
  std::string model_name;
  ModelStatus status = ModelStatus::Empty;
  ObjSense obj_sense = ObjSense::Minimize;
  std::vector<Variable> variables;
  // sparse objective coefficients for each column index
  std::map<ChenInt, double> objective_coef;
  double objective_offset = 0.0;
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

#endif // CPPOR_MODEL_H
