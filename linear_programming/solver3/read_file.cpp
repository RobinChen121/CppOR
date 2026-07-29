/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 24/06/2026, 22:59
 * Description:
 *
 */

#include "read_file.h"
#include "model.h"

#include <fstream>
#include <iostream>
#include <ranges>
#include <string>

// 包在这个 namespace 里面让这些函数只能在这个 cpp 文件里用
namespace {
// basic utilities

// trim is to remove the spaces in the front and end of a string
std::string trim(const std::string &s) {
  size_t first = 0;
  while (first < s.size() && std::isspace(static_cast<unsigned char>(s[first])))
    ++first;
  size_t last = s.size();
  while (last > first && std::isspace(static_cast<unsigned char>(s[last - 1])))
    --last;
  return s.substr(first, last - first);
}

std::string toLower(std::string s) {
  for (char &ch : s)
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  return s;
}

bool isNumberStart(const char ch) {
  return std::isdigit(static_cast<unsigned char>(ch)) || ch == '.';
}

// 变量名可以是：字母、数字、下划线 _、小数点 . 以及中括号 [ 和 ]
bool isVarChar(const char ch) {
  return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '.' || ch == '[' ||
         ch == ']';
}

bool startsWithWord(const std::string &line, const std::string &word) {
  const std::string lower = toLower(trim(line));
  if (lower.rfind(word, 0) != 0)
    return false;
  return lower.size() == word.size() ||
         std::isspace(static_cast<unsigned char>(lower[word.size()]));
}

// text processing

// 剥离 comments
std::string stripLpComment(const std::string &line) {
  // 下面的两个\\ 表示一个斜杠
  if (const size_t pos = line.find('\\'); pos != std::string::npos)
    return line.substr(0, pos);
  return line;
}

// 识别并剥离 LP（或 MPS）文件中的“可选标签/名称”（Label，例如 c1:,obj:），只保留实际的数学表达式
std::string removeOptionalLabel(const std::string &line) {
  const size_t colon = line.find(':');
  if (colon == std::string::npos)
    return line;

  // 提取冒号前面的文本，并去除前后空格
  const std::string before = trim(line.substr(0, colon));
  // 校验冒号前的文本是不是一个合法的“标签名”
  // 查找 before 中是否包含任何数学运算符/比较符 (+, -, *, <, =, >)
  if (before.find_first_of("+-*<=>") == std::string::npos)
    // 如果不包含这些运算符，说明 before 是一个纯粹的标签名（如 "c1" 或 "cost"）
    // 于是把冒号及冒号左边的标签剥离掉，只返回冒号后面的表达式内容
    return trim(line.substr(colon + 1));

  // 如果冒号前面包含了运算符，说明这个冒号可能不是标签分隔符（或者是非法的/特殊情况）
  // 为了安全起见，不作修改，原样返回
  return line;
}

// 将字符串解析为浮点型
double parseDoubleToken(std::string token) {
  token = toLower(trim(token));
  if (token == "inf" || token == "+inf" || token == "infinity" || token == "+infinity")
    return INF;
  if (token == "-inf" || token == "-infinity")
    return -INF;
  size_t idx = 0;
  const double value = std::stod(token, &idx);
  if (idx != token.size()) { // 避免 10abc 也被解析为 10 正确返回的情况
    throw std::runtime_error("Invalid numeric token: " + token);
  }

  return value;
}

void setLpBound(Model &model, const ChenInt col, const double lower, const double upper) {
  // model.free_var[col] = is_free;
  model.variables[col].lb = lower;
  model.variables[col].ub = upper;
}

std::vector<LinearTerm> parseLinearExpression(const std::string &expr, Model &model) {
  std::map<ChenInt, double> coefficients;
  size_t pos = 0;

  while (pos < expr.size()) {
    while (pos < expr.size() && std::isspace(static_cast<unsigned char>(expr[pos])))
      ++pos;
    if (pos >= expr.size())
      break;

    double sign = 1.0;
    if (expr[pos] == '+') {
      ++pos;
    } else if (expr[pos] == '-') {
      sign = -1.0;
      ++pos;
    }

    while (pos < expr.size() && std::isspace(static_cast<unsigned char>(expr[pos])))
      ++pos;

    double coef = 1.0;
    if (pos < expr.size() && isNumberStart(expr[pos])) {
      // 提取并转换变量前面的浮点数
      const char *start = expr.c_str() + pos; // 指针
      char *end = nullptr;
      // std::strtod（String to Double）是 C/C++
      // 标准库函数，它会自动跳过数字，将能识别的最大浮点数字符串转换为 double
      // 遇到非数字字符立即停下
      coef = std::strtod(start, &end);
      if (end != start) {
        pos += static_cast<size_t>(end - start);
      }
    }

    // 允许变量后面的空格
    while (pos < expr.size() && std::isspace(static_cast<unsigned char>(expr[pos])))
      ++pos;

    // 提取变量名的范围
    const size_t name_start = pos;
    while (pos < expr.size() &&
           isVarChar(expr[pos])) // 遇到 * 号 将会抛出错误，这个 * 号 没必要处理
      ++pos;
    if (name_start == pos) {
      throw std::runtime_error("Unexpected character '" + std::string(1, expr[pos]) +
                               "' in linear expression: " + expr);
    }

    const std::string name = expr.substr(name_start, pos - name_start);
    // 合并可能的重复变量
    coefficients[model.findOrCreateVariable(name)] += sign * coef;
  }

  std::vector<LinearTerm> terms;
  terms.reserve(coefficients.size());

  for (const auto &[col, coef] : coefficients) {
    if (std::abs(coef) > 1e-12) { // 可选：去掉合并后的零系数
      terms.push_back({col, coef});
    }
  }

  return terms;
}

void parseLpBoundLine(const std::string &line, Model &model) {
  std::string s = removeOptionalLabel(trim(line));
  if (s.empty())
    return;

  if (const std::string lower = toLower(s); lower.ends_with(" free")) {
    const std::string name = trim(s.substr(0, s.size() - 5));
    setLpBound(model, model.findOrCreateVariable(name), -INF, INF);
    return;
  }

  const size_t first_le = s.find("<=");
  const size_t first_ge = s.find(">=");
  if (first_le != std::string::npos && s.find("<=", first_le + 2) != std::string::npos) {
    const size_t second_le = s.find("<=", first_le + 2);
    const double lb = parseDoubleToken(s.substr(0, first_le));
    const std::string name = trim(s.substr(first_le + 2, second_le - first_le - 2));
    const double ub = parseDoubleToken(s.substr(second_le + 2));
    setLpBound(model, model.findOrCreateVariable(name), lb, ub);
    return;
  }
  if (first_ge != std::string::npos && s.find(">=", first_ge + 2) != std::string::npos) {
    const size_t second_ge = s.find(">=", first_ge + 2);
    const double ub = parseDoubleToken(s.substr(0, first_ge));
    const std::string name = trim(s.substr(first_ge + 2, second_ge - first_ge - 2));
    const double lb = parseDoubleToken(s.substr(second_ge + 2));
    setLpBound(model, model.findOrCreateVariable(name), lb, ub);
    return;
  }

  if (const size_t eq = s.find('='); eq != std::string::npos && s.find("<=") == std::string::npos &&
                                     s.find(">=") == std::string::npos) {
    const std::string name = trim(s.substr(0, eq));
    const double value = parseDoubleToken(s.substr(eq + 1));
    setLpBound(model, model.findOrCreateVariable(name), value, value);
    return;
  }

  const size_t op = first_le != std::string::npos ? first_le : first_ge;
  if (op == std::string::npos) {
    throw std::runtime_error("Invalid bound line: " + line);
  }

  const bool is_le = first_le != std::string::npos;
  const std::string left = trim(s.substr(0, op));
  const std::string right = trim(s.substr(op + 2));
  const bool left_is_number = !left.empty() && (std::isdigit(static_cast<unsigned char>(left[0])) ||
                                                left[0] == '-' || left[0] == '+' || left[0] == '.');

  if (left_is_number) {
    const double value = parseDoubleToken(left);
    const ChenInt col = model.findOrCreateVariable(right);
    if (is_le)
      model.variables[col].lb = value;
    else
      model.variables[col].ub = value;
  } else {
    const ChenInt col = model.findOrCreateVariable(left);
    const double value = parseDoubleToken(right);
    if (is_le)
      model.variables[col].ub = value;
    else
      model.variables[col].lb = value;
  }
}

} // namespace

// LP 文件一般没有模型的名字
Model readLP(const std::string &path) {
  std::ifstream file(path);
  if (!file)
    throw std::runtime_error("Cannot open LP file: " + path);

  Model model;
  enum class Section { None, Objective, Constraints, Bounds, Generals, Binaries };
  auto section = Section::None;

  std::string line;
  std::string obj_buffer; // 目标函数拼接缓冲区
  std::string con_buffer; // 跨行约束条件拼接缓冲区

  // 辅助 lambda：解析并清空目标函数缓冲区
  auto flushObjective = [&]() {
    std::string text = trim(obj_buffer);
    if (!text.empty()) {
      for (auto &[index, coef] : parseLinearExpression(removeOptionalLabel(text), model)) {
        model.objective_coef[index] += coef;
      }
      obj_buffer.clear();
    }
  };

  // 辅助 lambda：检查并解析拼接好的单个约束条件
  auto tryParseConstraint = [&](std::string &text) {
    text = trim(text);
    if (text.empty())
      return;

    // 检查是否包含完整的关系运算符
    size_t pos = std::string::npos;
    std::string operator_;
    if ((pos = text.find("<=")) != std::string::npos) {
      operator_ = "<=";
    } else if ((pos = text.find(">=")) != std::string::npos) {
      operator_ = ">=";
    } else if ((pos = text.find('=')) != std::string::npos) {
      operator_ = "=";
    } else {
      // 还没拼接完整，等待下一行输入
      return;
    }

    // 提取约束名称（如果有）
    std::string con_name;
    size_t name_pos = text.find(':');
    if (name_pos == std::string::npos)
      name_pos = 0;
    if (name_pos != 0 && name_pos < pos)
      con_name = trim(text.substr(0, name_pos));

    const std::string lhs_text = removeOptionalLabel(text.substr(name_pos, pos - name_pos));
    const double rhs = parseDoubleToken(text.substr(pos + operator_.size()));
    const int sense = operator_ == "<=" ? 0 : (operator_ == ">=" ? 1 : 2);

    if (sense == 0)
      model.addConstraint(con_name, parseLinearExpression(lhs_text, model), -INF, rhs);
    else if (sense == 1)
      model.addConstraint(con_name, parseLinearExpression(lhs_text, model), rhs, INF);
    else
      model.addConstraint(con_name, parseLinearExpression(lhs_text, model), rhs, rhs);

    // 解析完成后清空缓冲区
    text.clear();
  };

  // getline is defined in <string, ftream, stream>: reads characters from an input stream and
  // places them into a string until \n or end of file
  // 每次读取一行，直到文件末尾循环结束
  while (std::getline(file, line)) {
    line = trim(stripLpComment(line)); // 去掉注释以及两边的空格
    if (line.empty())
      continue;

    const std::string lower = toLower(line);
    if (startsWithWord(line, "minimize") || startsWithWord(line, "minimum") ||
        startsWithWord(line, "min")) {
      model.obj_sense = ObjSense::Minimize;
      section = Section::Objective;
      const size_t space = line.find_first_of(" \t"); // 找到第一个 tab 位
      if (space != std::string::npos)
        obj_buffer += " " + trim(line.substr(space + 1)); // 去掉前面的 min 然后拼接后面可能的公式
      continue;
    } else if (startsWithWord(line, "maximize") || startsWithWord(line, "maximum") ||
               startsWithWord(line, "max")) {
      model.obj_sense = ObjSense::Maximize;
      section = Section::Objective;
      const size_t space = line.find_first_of(" \t"); // return the position of the found character
                                                      // or npos if no such character is found.
      if (space != std::string::npos)
        obj_buffer += " " + trim(line.substr(space + 1));
      continue;
    } else if (startsWithWord(line, "subject to") || startsWithWord(line, "such that") ||
               startsWithWord(line, "s.t.") || startsWithWord(line, "st")) {
      flushObjective(); // 解析并清空目标函数缓冲区
      section = Section::Constraints;
      continue;
    } else if (startsWithWord(line, "bounds")) {
      section = Section::Bounds;
      continue;
    } else if (startsWithWord(line, "general") || startsWithWord(line, "generals")) {

      section = Section::Generals;
      continue;
    } else if (startsWithWord(line, "binary") || startsWithWord(line, "binaries") ||
               startsWithWord(line, "bin")) {

      section = Section::Binaries;
      continue;
    } else if (startsWithWord(line, "end")) {
      break;
    }

    line = trim(line);
    if (line.empty())
      continue;

    if (section == Section::Objective) {
      for (auto obj_formula = parseLinearExpression(removeOptionalLabel(line), model);
           auto &[index, coef] : obj_formula) {
        model.objective_coef[index] += coef;
      }
      obj_buffer += " " + line; // 目标函数拼接缓冲区
    } else if (section == Section::Constraints) {
      // 若 con_buffer 不为空，当前新行不应该包含了新的约束名 (例如 "c2:")
      if (!con_buffer.empty() && line.find(':') != std::string::npos &&
          con_buffer.find("<=") == std::string::npos &&
          con_buffer.find(">=") == std::string::npos && con_buffer.find('=') == std::string::npos) {
        throw std::runtime_error("Invalid constraint structure near: " + con_buffer);
      }

      // 追加当前行到约束缓冲区
      con_buffer += " " + line;

      // 尝试解析，只有检测到关系运算符和 RHS 时才会真正触发解析并清空 con_buffer
      tryParseConstraint(con_buffer);

    } else if (section == Section::Bounds) {
      parseLpBoundLine(line, model);
    } else if (section == Section::Generals) {

      std::istringstream in(line); // 逐个读取一行文本中的单词

      std::string name;
      while (in >> name) {
        ChenInt col = model.findOrCreateVariable(name);
        model.variables[col].var_type = VarType::Integer;
      }
    } else if (section == Section::Binaries) {

      std::istringstream in(line);
      std::string name;
      while (in >> name) {
        ChenInt col = model.findOrCreateVariable(name);
        model.variables[col].var_type = VarType::Binary;
        model.variables[col].lb = 0.0;
        model.variables[col].ub = 1.0;
      }
    }
  }

  return model;
}

Model readMPS(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open())
    throw std::runtime_error("Cannot open MPS file: " + path);

  enum class Section { NONE, OBJSENSE, ROWS, COLUMNS, RHS, RANGES, BOUNDS };
  auto section = Section::NONE;

  struct RowInfo {
    char type{};
    std::string name;
  };

  struct BoundInfo {
    double lb = 0.0;
    double ub = INF;
    bool is_explicit_free = false;
    bool is_binary = false;
  };

  //----------------------------------------
  // temporary storage
  //----------------------------------------
  std::vector<RowInfo> rows;
  std::string objective_name;
  std::vector<std::string> col_names;
  std::map<std::string, std::map<std::string, double>> columns;                 // 用于 objective
  std::map<std::string, std::vector<std::pair<std::string, double>>> row_terms; // 用于 constraints
  std::map<std::string, double> rhs_values;
  std::map<std::string, double> ranges_values; // 储存 RANGES
  std::map<std::string, BoundInfo> bounds_map;
  std::map<std::string, bool> is_int_marker;

  //----------------------------------------
  Model model;
  model.obj_sense = ObjSense::Minimize;
  std::string line;
  bool in_integer_section = false;

  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '*')
      continue;
    std::istringstream iss(line); // isstringstream 可以从 string 里提取单词
    std::string first;
    if (!(iss >> first))
      continue;

    //----------------------------------------
    // section switching
    //----------------------------------------
    if (first == "NAME") {
      model.model_name = trim(line.substr(5));
      continue;
    }
    if (first == "OBJSENSE") {
      section = Section::OBJSENSE;
      continue;
    }
    if (first == "ROWS") {
      section = Section::ROWS;
      continue;
    }
    if (first == "COLUMNS") {
      section = Section::COLUMNS;
      continue;
    }
    if (first == "RHS") {
      section = Section::RHS;
      continue;
    }
    if (first == "RANGES") {
      section = Section::RANGES;
      continue;
    }
    if (first == "BOUNDS") {
      section = Section::BOUNDS;
      continue;
    }
    if (first == "ENDATA")
      break;

    //----------------------------------------
    // OBJSENSE
    //----------------------------------------
    if (section == Section::OBJSENSE) {
      if (first == "MAX" || first == "MAXIMIZE")
        model.obj_sense = ObjSense::Maximize;
      else if (first == "MIN" || first == "MINIMIZE")
        model.obj_sense = ObjSense::Minimize;
    }

    //----------------------------------------
    // ROWS
    //----------------------------------------
    else if (section == Section::ROWS) {
      char type = first[0];
      std::string row_name;
      iss >> row_name;
      rows.push_back({type, row_name});
      if (type == 'N' && objective_name.empty()) {
        objective_name = row_name;
      }
    }

    //----------------------------------------
    // COLUMNS
    //----------------------------------------
    else if (section == Section::COLUMNS) {
      const std::string &col = first; // 引用绑定，数组长时效率提升明显
      std::string row1;
      iss >> row1;
      // 整数变量
      if (row1 == "'MARKER'") {
        std::string marker_type;
        iss >> marker_type;
        // 整数变量开始
        if (marker_type == "'INTORG'")
          in_integer_section = true;
        // 整数变量结束
        else if (marker_type == "'INTEND'")
          in_integer_section = false;
        continue;
      }

      if (!columns.contains(col))
        col_names.push_back(col);

      if (in_integer_section)
        is_int_marker[col] = true;

      double val1{};
      iss >> val1;
      // 用 += 是因为有时候变量重复
      columns[col][row1] += val1;
      if (row1 != objective_name)
        // 当需要向容器中新增一个需要现场构造的对象时（直接传入构造函数的参数），优先使用
        // emplace_back, 能够省去了移动/拷贝的开销
        row_terms[row1].emplace_back(col, val1);

      double val2{};

      if (std::string row2; iss >> row2 >> val2) {
        columns[col][row2] += val2;
        if (row2 != objective_name)
          row_terms[row2].emplace_back(col, val2);
      }
    }

    //----------------------------------------
    // RHS
    //----------------------------------------
    else if (section == Section::RHS) {
      // const std::string &rhs_name = first;
      // (void)rhs_name; // 消除编译器 "Unused variable"（未使用的变量）警告 的经典技巧

      double val1{};

      if (std::string row1; iss >> row1 >> val1)
        rhs_values[row1] += val1;

      double val2{};

      if (std::string row2; iss >> row2 >> val2)
        rhs_values[row2] += val2;
    }

    //----------------------------------------
    // RANGES
    //----------------------------------------
    else if (section == Section::RANGES) {
      // std::string range_name = first;
      std::string row1;
      if (double val1{}; iss >> row1 >> val1)
        ranges_values[row1] = val1;

      double val2{};
      if (std::string row2; iss >> row2 >> val2)
        ranges_values[row2] = val2;
    }

    //----------------------------------------
    // BOUNDS
    //----------------------------------------
    else if (section == Section::BOUNDS) {
      const std::string &btype = first;

      std::string bound_name;
      std::string col;

      iss >> bound_name >> col;
      auto &[lb, ub, is_explicit_free, is_binary] = bounds_map[col];

      if (btype == "FR") {
        lb = -INF;
        ub = INF;
        is_explicit_free = true;
      }

      else if (btype == "BV") {
        lb = 0.0;
        ub = 1.0;
        is_binary = true;
      }

      else if (btype == "MI") {
        lb = -INF;
        if (ub == INF)
          ub = 0.0;
      }

      else if (btype == "PL") {
        lb = 0.0;
        ub = INF;
      }

      else {
        double value{};
        if (!(iss >> value))
          continue;
        if (btype == "LO" || btype == "LI")
          lb = value;
        else if (btype == "UP" || btype == "UI")
          ub = value;
        else if (btype == "FX") {
          lb = value;
          ub = value;
        }
      }
    }
  }

  //----------------------------------------
  // create variables
  //----------------------------------------
  for (const auto &col_name : col_names) {
    double lb = 0.0;
    double ub = INF;
    auto type = VarType::Continuous;

    if (const auto it = bounds_map.find(col_name); it != bounds_map.end()) {
      lb = it->second.lb;
      ub = it->second.ub;
      if (it->second.is_binary)
        type = VarType::Binary;
    }

    if (type == VarType::Continuous && is_int_marker.contains(col_name))
      type = VarType::Integer;

    std::string name = col_name;
    model.addVariable(name, lb, ub, type);
  }

  //----------------------------------------
  // objective
  //----------------------------------------
  for (const auto &col_name : col_names) {
    auto col_it = columns.find(col_name);
    if (col_it == columns.end())
      continue;

    auto obj_it = col_it->second.find(objective_name);
    if (obj_it == col_it->second.end())
      continue;

    ChenInt col = model.findOrCreateVariable(col_name);
    model.objective_coef[col] += obj_it->second;
  }

  //----------------------------------------
  // constraints (包含处理 ranges 上限 与 下限)
  //----------------------------------------
  // 这样循环会降低时间复杂度，从 num_rows*num_cols 降为 num_non_zeros
  for (const auto &[type, name] : rows) {
    if (name == objective_name)
      continue;

    std::vector<LinearTerm> terms;
    if (auto row_it = row_terms.find(name); row_it != row_terms.end()) {
      terms.reserve(row_it->second.size());
      for (const auto &[col_name, coef] : row_it->second) {
        terms.push_back({model.findOrCreateVariable(col_name), coef});
      }
    }

    double rhs = rhs_values.contains(name) ? rhs_values[name] : 0.0;
    bool has_range = ranges_values.contains(name);
    double range = has_range ? ranges_values[name] : 0.0;

    double lhs_bound = -INF;
    double rhs_bound = INF;

    switch (type) {
    case 'L': // Ax <= rhs，此时添加一个下界：rhs减去range
      rhs_bound = rhs;
      lhs_bound = has_range ? (rhs - std::abs(range)) : -INF;
      break;
    case 'G': // Ax >= rhs，此时添加一个上界：rhs加上range
      lhs_bound = rhs;
      rhs_bound = has_range ? (rhs + std::abs(range)) : INF;
      break;
    case 'E': // Ax = rhs 或 rhs <= Ax <= rhs + range，此时等式约束退化为区间约束
      if (!has_range) {
        lhs_bound = rhs;
        rhs_bound = rhs;
      } else if (range > 0) {
        lhs_bound = rhs;
        rhs_bound = rhs + range;
      } else {
        lhs_bound = rhs + range;
        rhs_bound = rhs;
      }
      break;
    default:
      break;
    }

    if (type != 'N') {
      model.addConstraint(name, terms, lhs_bound, rhs_bound);
    }
  }

  return model;
}

Model read(const std::string &path) {
  const auto pos = path.find_last_of('.');
  if (pos == std::string::npos)
    throw std::runtime_error("File has no extension: " + path);

  auto file_type = path.substr(pos);
  for (char &c : file_type) // 转小写
    c = static_cast<char>(std::tolower(c));
  if (file_type == ".mps")
    return readMPS(path);
  if (file_type == ".lp")
    return readLP(path);
  throw std::runtime_error("Unknown file type");
}

int main() {
  std::string file_path;
  // #ifdef 是 C/C++ 预处理器（Preprocessor）指令，_WIN32 为宏
#ifdef _WIN32
  file_path = "D:/chenzhen/CppOR/linear_programming/test_sets/boeing1.mps";
#else
  file_path = "/Users/zhenchen/CLionProjects/CppOR/linear_programming/test_sets/boeing1.lp";
#endif
  auto problem = read(file_path);
  problem.print();
}