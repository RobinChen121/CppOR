/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 24/06/2026, 22:59
 * Description:
 *
 */

#ifndef CHEN_SOLVER_JS_READ_FILE_H
#define CHEN_SOLVER_JS_READ_FILE_H

#include "config.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

struct ParsedLinearProgram {
  int obj_sense = 0;
  std::vector<std::string> var_names;
  std::map<std::string, int> var_index;
  std::map<int, double> objective;
  std::vector<std::map<int, double>> lhs;
  std::vector<double> rhs;
  std::vector<int> constraint_sense;
  std::vector<double> lower_bound;
  std::vector<double> upper_bound;
  std::vector<bool> free_var;

  int ensureVar(const std::string &name) {
    if (const auto it = var_index.find(name); it != var_index.end())
      return it->second;
    const int index = static_cast<int>(var_names.size());
    var_index[name] = index;
    var_names.push_back(name);
    lower_bound.push_back(0.0);
    upper_bound.push_back(INF);
    free_var.push_back(false);
    return index;
  }

  void addConstraint(const std::map<int, double> &row, const int sense, const double value) {
    lhs.push_back(row);
    constraint_sense.push_back(sense);
    rhs.push_back(value);
  }
};

static std::string trim(const std::string &s) {
  size_t first = 0;
  while (first < s.size() && std::isspace(static_cast<unsigned char>(s[first])))
    ++first;
  size_t last = s.size();
  while (last > first && std::isspace(static_cast<unsigned char>(s[last - 1])))
    --last;
  return s.substr(first, last - first);
}

static std::string toLower(std::string s) {
  for (char &ch : s)
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  return s;
}

static std::string toUpper(std::string s) {
  for (char &ch : s)
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
  return s;
}

static bool startsWithWord(const std::string &line, const std::string &word) {
  const std::string lower = toLower(trim(line));
  if (lower.rfind(word, 0) != 0)
    return false;
  return lower.size() == word.size() ||
         std::isspace(static_cast<unsigned char>(lower[word.size()]));
}

static bool isNumberStart(const char ch) {
  return std::isdigit(static_cast<unsigned char>(ch)) || ch == '.';
}

static bool isVarChar(const char ch) {
  return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '.' || ch == '[' ||
         ch == ']';
}

static std::vector<std::string> splitTokens(const std::string &line) {
  std::istringstream in(line);
  std::vector<std::string> tokens;
  std::string token;
  while (in >> token)
    tokens.push_back(token);
  return tokens;
}

static std::string stripLpComment(const std::string &line) {
  if (const size_t pos = line.find('\\'); pos != std::string::npos)
    return line.substr(0, pos);
  return line;
}

static std::string removeOptionalLabel(const std::string &line) {
  const size_t colon = line.find(':');
  if (colon == std::string::npos)
    return line;

  const std::string before = trim(line.substr(0, colon));
  if (before.find_first_of("+-*<=>") == std::string::npos)
    return trim(line.substr(colon + 1));
  return line;
}

static double parseDoubleToken(std::string token) {
  token = toLower(trim(token));
  if (token == "inf" || token == "+inf" || token == "infinity" || token == "+infinity")
    return INF;
  if (token == "-inf" || token == "-infinity")
    return -INF;
  return std::stod(token);
}

static std::map<int, double> parseLinearExpression(const std::string &expr,
                                                   ParsedLinearProgram &lp) {
  std::map<int, double> coefficients;
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

static void mergeCoefficients(std::map<int, double> &target, const std::map<int, double> &source) {
  for (const auto &[col, value] : source)
    target[col] += value;
}

static void setLpBound(ParsedLinearProgram &lp, const int col, const double lower,
                       const double upper, const bool is_free) {
  lp.free_var[col] = is_free;
  lp.lower_bound[col] = lower;
  lp.upper_bound[col] = upper;
}

static void parseLpBoundLine(const std::string &line, ParsedLinearProgram &lp) {
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

static ParsedLinearProgram parseLpFileData(const std::string &path) {
  std::ifstream file(path);
  if (!file)
    throw std::runtime_error("Cannot open LP file: " + path);

  ParsedLinearProgram lp;
  enum class Section { None, Objective, Constraints, Bounds };
  Section section = Section::None;
  std::string line;

  while (std::getline(file, line)) {
    line = trim(stripLpComment(line));
    if (line.empty())
      continue;

    const std::string lower = toLower(line);
    if (startsWithWord(line, "minimize") || startsWithWord(line, "minimum") ||
        startsWithWord(line, "min")) {
      lp.obj_sense = 0;
      section = Section::Objective;
      const size_t space = line.find_first_of(" \t");
      line = space == std::string::npos ? "" : trim(line.substr(space + 1));
    } else if (startsWithWord(line, "maximize") || startsWithWord(line, "maximum") ||
               startsWithWord(line, "max")) {
      lp.obj_sense = 1;
      section = Section::Objective;
      const size_t space = line.find_first_of(" \t");
      line = space == std::string::npos ? "" : trim(line.substr(space + 1));
    } else if (startsWithWord(line, "subject to") || startsWithWord(line, "such that") ||
               startsWithWord(line, "s.t.") || startsWithWord(line, "st")) {
      section = Section::Constraints;
      continue;
    } else if (startsWithWord(line, "bounds")) {
      section = Section::Bounds;
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
    }
  }

  return lp;
}

#endif // CHEN_SOLVER_JS_READ_FILE_H
