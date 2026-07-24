/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 24/06/2026, 22:59
 * Description:
 *
 */

#include "read_file.h"

#include "util.h"

#include <algorithm>
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

bool isInteger(const std::string &str) {
  return std::ranges::all_of(
      str, [](const char ch) { return std::isdigit(static_cast<unsigned char>(ch)); });

  // lambda 函数可以实现函数参数隐式类型转换（Implicit Parameter Conversion）
  // 与上面的等价
  // return std::ranges::all_of(str, [](const unsigned char ch) { return std::isdigit(ch); });
}

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
  if (const size_t pos = line.find('\\'); pos != std::string::npos)
    return line.substr(0, pos);
  return line;
}

// 识别并剥离 LP（或 MPS）文件中的“可选标签/名称”（Label），只保留实际的数学表达式
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
  return std::stod(token);
}

// LP parsing helpers

void mergeCoefficients(std::unordered_map<int, double> &target,
                       const std::unordered_map<int, double> &source) {
  for (const auto &[col, value] : source)
    target[col] += value;
}

void setLpBound(ParsedModel &lp, const int col, const double lower, const double upper,
                const bool is_free) {
  lp.free_var[col] = is_free;
  lp.lower_bound[col] = lower;
  lp.upper_bound[col] = upper;
}

std::unordered_map<int, double> parseLinearExpression(const std::string &expr, ParsedModel &lp) {
  std::unordered_map<int, double> coefficients;
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
    bool has_coef = false;
    if (pos < expr.size() && isNumberStart(expr[pos])) {
      const char *start = expr.c_str() + pos;
      char *end = nullptr;
      coef = std::strtod(start, &end);
      if (end != start) {
        has_coef = true;
        pos += static_cast<size_t>(end - start);
      }
    }

    while (pos < expr.size() && std::isspace(static_cast<unsigned char>(expr[pos])))
      ++pos;
    if (pos < expr.size() && expr[pos] == '*')
      ++pos;
    while (pos < expr.size() && std::isspace(static_cast<unsigned char>(expr[pos])))
      ++pos;

    const size_t name_start = pos;
    while (pos < expr.size() && isVarChar(expr[pos]))
      ++pos;

    if (name_start == pos) {
      if (!has_coef)
        ++pos;
      continue;
    }

    const std::string name = expr.substr(name_start, pos - name_start);
    coefficients[lp.ensureVar(name)] += sign * coef;
  }

  return coefficients;
}

void parseLpBoundLine(const std::string &line, ParsedModel &lp) {
  std::string s = removeOptionalLabel(trim(line));
  if (s.empty())
    return;

  const std::string lower = toLower(s);
  if (lower.ends_with(" free")) {
    const std::string name = trim(s.substr(0, s.size() - 5));
    setLpBound(lp, lp.ensureVar(name), -INF, INF, true);
    return;
  }

  const size_t first_le = s.find("<=");
  const size_t first_ge = s.find(">=");
  if (first_le != std::string::npos && s.find("<=", first_le + 2) != std::string::npos) {
    const size_t second_le = s.find("<=", first_le + 2);
    const double lb = parseDoubleToken(s.substr(0, first_le));
    const std::string name = trim(s.substr(first_le + 2, second_le - first_le - 2));
    const double ub = parseDoubleToken(s.substr(second_le + 2));
    setLpBound(lp, lp.ensureVar(name), lb, ub, false);
    return;
  }
  if (first_ge != std::string::npos && s.find(">=", first_ge + 2) != std::string::npos) {
    const size_t second_ge = s.find(">=", first_ge + 2);
    const double ub = parseDoubleToken(s.substr(0, first_ge));
    const std::string name = trim(s.substr(first_ge + 2, second_ge - first_ge - 2));
    const double lb = parseDoubleToken(s.substr(second_ge + 2));
    setLpBound(lp, lp.ensureVar(name), lb, ub, false);
    return;
  }

  const size_t eq = s.find('=');
  if (eq != std::string::npos && s.find("<=") == std::string::npos &&
      s.find(">=") == std::string::npos) {
    const std::string name = trim(s.substr(0, eq));
    const double value = parseDoubleToken(s.substr(eq + 1));
    setLpBound(lp, lp.ensureVar(name), value, value, false);
    return;
  }

  const size_t op = first_le != std::string::npos ? first_le : first_ge;
  if (op == std::string::npos)
    return;

  const bool is_le = first_le != std::string::npos;
  const std::string left = trim(s.substr(0, op));
  const std::string right = trim(s.substr(op + 2));
  const bool left_is_number = !left.empty() && (std::isdigit(static_cast<unsigned char>(left[0])) ||
                                                left[0] == '-' || left[0] == '+' || left[0] == '.');

  if (left_is_number) {
    const double value = parseDoubleToken(left);
    const int col = lp.ensureVar(right);
    if (is_le)
      lp.lower_bound[col] = value;
    else
      lp.upper_bound[col] = value;
  } else {
    const int col = lp.ensureVar(left);
    const double value = parseDoubleToken(right);
    if (is_le)
      lp.upper_bound[col] = value;
    else
      lp.lower_bound[col] = value;
  }
}

} // namespace

ParsedModel readLP(const std::string &path) {
  std::ifstream file(path);
  if (!file)
    throw std::runtime_error("Cannot open LP file: " + path);

  ParsedModel lp;
  enum class Section { None, Objective, Constraints, Bounds, Generals, Binaries };
  auto section = Section::None;
  std::string line;

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
      lp.obj_sense = 0;
      section = Section::Objective;
      const size_t space = line.find_first_of(" \t"); // 找到第一个 tab 位
      line = space == std::string::npos ? "" : trim(line.substr(space + 1));
    } else if (startsWithWord(line, "maximize") || startsWithWord(line, "maximum") ||
               startsWithWord(line, "max")) {
      lp.obj_sense = 1;
      section = Section::Objective;
      const size_t space = line.find_first_of(" \t"); // return the position of the found character
                                                      // or npos if no such character is found.
      line = space == std::string::npos ? "" : trim(line.substr(space + 1));
    } else if (startsWithWord(line, "subject to") || startsWithWord(line, "such that") ||
               startsWithWord(line, "s.t.") || startsWithWord(line, "st")) {
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
      mergeCoefficients(lp.objective, parseLinearExpression(removeOptionalLabel(line), lp));
    } else if (section == Section::Constraints) {
      std::string op;
      size_t pos = line.find("<=");
      if (pos != std::string::npos) {
        op = "<=";
      } else if ((pos = line.find(">=")) != std::string::npos) {
        op = ">=";
      } else if ((pos = line.find('=')) != std::string::npos) {
        op = "=";
      } else {
        continue;
      }

      const std::string lhs_text = removeOptionalLabel(line.substr(0, pos));
      const double value = parseDoubleToken(line.substr(pos + op.size()));
      const int sense = op == "<=" ? 0 : (op == ">=" ? 1 : 2);
      lp.addConstraint(parseLinearExpression(lhs_text, lp), sense, value);
    } else if (section == Section::Bounds) {
      parseLpBoundLine(line, lp);
    } else if (section == Section::Generals) {

      std::istringstream in(line); // 逐个读取一行文本中的单词

      std::string name;

      while (in >> name) {
        int col = lp.ensureVar(name);
        lp.var_type[col] = 1;
      }
    } else if (section == Section::Binaries) {

      std::istringstream in(line);

      std::string name;

      while (in >> name) {
        int col = lp.ensureVar(name);
        lp.var_type[col] = 2;
        lp.lower_bound[col] = 0.0;
        lp.upper_bound[col] = 1.0;
      }
    }
  }

  return lp;
}

ParsedModel readMPS(const std::string &path) {
  std::ifstream file(path);

  if (!file.is_open())
    throw std::runtime_error("Cannot open MPS file: " + path);

  enum class Section { NONE, OBJSENSE, ROWS, COLUMNS, RHS, BOUNDS };
  auto section = Section::NONE;

  struct RowInfo {
    char type;
    std::string name;
  };

  std::vector<RowInfo> rows;
  std::string objective_name;

  // 临时存储解析数据
  std::vector<std::string> col_names;
  std::unordered_map<std::string, std::unordered_map<std::string, double>> columns;
  std::unordered_map<std::string, double> rhs_values;

  struct BoundInfo {
    double lb = 0.0;
    double ub = INF;
    bool is_explicit_free = false;
    bool is_binary = false;
  };
  std::unordered_map<std::string, BoundInfo> bounds_map;
  std::unordered_map<std::string, bool> is_int_marker; // 标记 COLUMNS 中的 MARKER 变量

  ParsedModel lp;
  std::string line;
  bool in_integer_section = false; // 是否在 MARKER 整数块内

  while (std::getline(file, line)) {
    // 跳过空行和注释行（* 开头）
    if (line.empty() || line[0] == '*')
      continue;

    std::istringstream iss(line);
    std::string first;
    iss >> first;

    // Section 切换判断
    if (first == "NAME")
      continue;
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
    if (first == "BOUNDS") {
      section = Section::BOUNDS;
      continue;
    }
    if (first == "ENDATA")
      break;

    // 解析 OBJSENSE (部分扩展支持)
    if (section == Section::OBJSENSE) {
      if (first == "MAX" || first == "MAXIMIZE")
        lp.obj_sense = 1;
      else if (first == "MIN" || first == "MINIMIZE")
        lp.obj_sense = 0;
    }
    // 解析 ROWS
    else if (section == Section::ROWS) {
      char type = first[0];
      std::string row_name;
      iss >> row_name;
      rows.push_back({type, row_name});

      if (type == 'N' && objective_name.empty()) {
        objective_name = row_name; // 记录目标函数行名
      }
    }
    // 解析 COLUMNS (包含 MARKER 处理)
    else if (section == Section::COLUMNS) {
      std::string col = first;
      std::string row1;
      iss >> row1;

      // 检查是否为 INTORG / INTEND 整数区间标记
      if (row1 == "'MARKER'") {
        std::string marker_type;
        iss >> marker_type;
        if (marker_type == "'INTORG'")
          in_integer_section = true;
        else if (marker_type == "'INTEND'")
          in_integer_section = false;
        continue;
      }

      double val1;
      iss >> val1;

      // 记录变量出现顺序
      if (!columns.contains(col)) {
        col_names.push_back(col);
        if (in_integer_section) {
          is_int_marker[col] = true;
        }
      }

      columns[col][row1] = val1;

      std::string row2;
      double val2;
      if (iss >> row2 >> val2) {
        columns[col][row2] = val2;
      }
    }
    // 解析 RHS
    else if (section == Section::RHS) {
      std::string rhs_name = first; // 忽略向量名

      std::string row1;
      double val1;
      if (iss >> row1 >> val1) {
        rhs_values[row1] = val1;
      }

      std::string row2;
      double val2;
      if (iss >> row2 >> val2) {
        rhs_values[row2] = val2;
      }
    }
    // 解析 BOUNDS
    else if (section == Section::BOUNDS) {
      std::string btype = first;
      std::string bname, col;
      iss >> bname >> col;

      auto &bnd = bounds_map[col];

      if (btype == "FR") { // Free 变量 (-INF, +INF)
        bnd.lb = -INF;
        bnd.ub = INF;
        bnd.is_explicit_free = true;
      } else if (btype == "BV") { // Binary 变量 [0, 1]
        bnd.lb = 0.0;
        bnd.ub = 1.0;
        bnd.is_binary = true;
      } else if (btype == "MI") { // Minus infinity (-INF, 0]
        bnd.lb = -INF;
        if (bnd.ub == INF)
          bnd.ub = 0.0;
      } else if (btype == "PL") { // Plus infinity [0, +INF)
        bnd.lb = 0.0;
        bnd.ub = INF;
      } else {
        double value;
        if (iss >> value) {
          if (btype == "LO" || btype == "LI")
            bnd.lb = value;
          else if (btype == "UP" || btype == "UI")
            bnd.ub = value;
          else if (btype == "FX") {
            bnd.lb = value;
            bnd.ub = value;
          }
        }
      }
    }
  }

  //----------------------------------------
  // 构建映射与约束信息
  //----------------------------------------
  std::unordered_map<std::string, int> row_id;
  std::vector<RowInfo> constraints;

  for (auto &r : rows) {
    if (r.name == objective_name)
      continue; // 排除目标函数号
    row_id[r.name] = static_cast<int>(constraints.size());
    constraints.push_back(r);
  }

  int num_rows = static_cast<int>(constraints.size());
  int num_cols = static_cast<int>(col_names.size());

  // 初始化容器尺寸
  lp.var_names = col_names;
  lp.lhs.resize(num_rows);
  lp.rhs.resize(num_rows, 0.0);
  lp.constraint_sense.resize(num_rows, 0);

  lp.var_type.resize(num_cols, 0);
  lp.lower_bound.resize(num_cols, 0.0);
  lp.upper_bound.resize(num_cols, INF);
  lp.free_var.resize(num_cols, false);

  // 填充约束条件 (RHS 与 Sense)
  for (int i = 0; i < num_rows; ++i) {
    const auto &[type, name] = constraints[i];

    if (type == 'L')
      lp.constraint_sense[i] = 0; // <=
    else if (type == 'E')
      lp.constraint_sense[i] = 1; // =
    else if (type == 'G')
      lp.constraint_sense[i] = 2; // >=

    if (rhs_values.contains(name)) {
      lp.rhs[i] = rhs_values[name];
    }
  }

  // 填充变量信息、目标函数以及 LHS 稀疏矩阵
  for (int j = 0; j < num_cols; ++j) {
    const std::string &col_name = col_names[j];
    lp.var_index[col_name] = j;

    // --- 处理 Bounds 与 变量类型 ---
    double lb = 0.0;
    double ub = INF;
    bool is_free = false;
    int vtype = 0; // 0: continuous

    // 检查 MARKER 标记的整数
    if (is_int_marker.contains(col_name) && is_int_marker[col_name]) {
      vtype = 1; // Integer
    }

    // 检查 BOUNDS 节显式定义
    if (bounds_map.contains(col_name)) {
      const auto &bnd = bounds_map[col_name];
      lb = bnd.lb;
      ub = bnd.ub;
      is_free = bnd.is_explicit_free || (lb == -INF && ub == INF);

      if (bnd.is_binary) {
        vtype = 2; // Binary
      } else if (vtype == 0 && (bnd.lb != 0.0 || bnd.ub != INF)) {
        // 如果原本不是 MARKER 整数，但有 UI/LI 标记，也可在此扩展判断
      }
    }

    lp.lower_bound[j] = lb;
    lp.upper_bound[j] = ub;
    lp.free_var[j] = is_free;
    lp.var_type[j] = vtype;

    // --- 处理 矩阵 A (lhs) 与 目标函数 (objective) ---
    for (const auto &[rname, val] : columns[col_name]) {
      if (rname == objective_name) {
        if (val != 0.0) {
          lp.objective[j] = val; // 目标函数非零项
        }
      } else if (row_id.contains(rname)) {
        int i = row_id[rname];
        lp.lhs[i][j] = val; // 给第 i 行约束，插入第 j 个变量的系数
      }
    }
  }

  return lp;
}

ParsedModel read(const std::string &path) {
  auto pos = path.find('.');
  auto file_type = path.substr(pos);
  for (char &c : file_type) // 转小写
    c = static_cast<char>(std::tolower(c));
  if (file_type == ".mps")
    return readMPS(path);
  else if (file_type == ".lp")
    return readLP(path);
  else
    throw std::runtime_error("Unknown file type");
}

void ParsedModel::print() {

  // 这个匿名函数里面的第一个 & 使得它可以访问匿名函数外面的函数或变量
  const auto printTerm = [&](const double coef, const std::string &name, bool &printed) {
    if (std::abs(coef) < EPS)
      return;

    if (printed) {
      std::cout << (coef < 0.0 ? " - " : " + ");
    } else if (coef < 0.0) {
      std::cout << "-";
    }

    const double abs_coef = std::abs(coef);

    if (std::abs(abs_coef - 1.0) > EPS)
      std::cout << abs_coef << " ";

    std::cout << name;
    printed = true;
  };

  const auto printExpr = [&](const std::unordered_map<int, double> &expr) {
    bool printed = false;

    for (const auto &[col, coef] : expr) {
      printTerm(coef, var_names[col], printed); // 饮用传递，可能会修改 printed
    }

    if (!printed)
      std::cout << "0";
  };

  const auto senseText = [](const int sense) {
    if (sense == 0)
      return " <= ";
    if (sense == 1)
      return " >= ";
    return " = ";
  };

  std::cout << "********************************\n";

  if (obj_sense == 0)
    std::cout << "Minimize\n";
  else
    std::cout << "Maximize\n";

  std::cout << " obj: ";
  printExpr(objective);
  std::cout << "\n\n";

  std::cout << "Subject To\n";

  for (size_t row = 0; row < lhs.size(); ++row) {
    std::cout << " c" << row + 1 << ": ";

    printExpr(lhs[row]);

    std::cout << senseText(constraint_sense[row]) << rhs[row] << "\n";
  }

  std::cout << "\nBounds\n";

  const size_t n = var_names.size();

  for (size_t i = 0; i < n; ++i) {

    const std::string &name = var_names[i];

    if (free_var[i]) {
      std::cout << " " << name << " free\n";
      continue;
    }

    const double lb = lower_bound[i];
    const double ub = upper_bound[i];

    if (std::abs(lb - ub) < EPS) {
      std::cout << " " << name << " = " << lb << "\n";
    } else {
      if (lb > -INF / 2 && ub < INF / 2) {
        std::cout << " " << lb << " <= " << name << " <= " << ub << "\n";
      } else if (lb > -INF / 2) {
        std::cout << " " << lb << " <= " << name << "\n";
      } else if (ub < INF / 2) {
        std::cout << " " << name << " <= " << ub << "\n";
      }
    }
  }

  std::cout << "\nGenerals\n";
  for (size_t i = 0; i < n; ++i) {
    if (var_type[i] == 1)
      std::cout << " " << var_names[i] << "\n";
  }

  std::cout << "\nBinaries\n";
  for (size_t i = 0; i < n; ++i) {
    if (var_type[i] == 2)
      std::cout << " " << var_names[i] << "\n";
  }

  std::cout << "\nEnd\n";
}

int main() {
  std::string file_path;
  // #ifdef 是 C/C++ 预处理器（Preprocessor）指令，_WIN32 为宏
#ifdef _WIN32
  file_path = "D:/chenzhen/CppOR/linear_programming/test_sets/boeing1.mps";
#endif
  file_path = "/Users/zhenchen/CLionProjects/CppOR/linear_programming/test_sets/afiro.lp";
  auto problem = read(file_path);
  problem.print();
}