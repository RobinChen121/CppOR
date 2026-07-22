/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 24/06/2026, 22:59
 * Description:
 *
 */

#include "read_file.h"
#include <string>

static ParsedLinearProgram parseLpFileData(const std::string &path) {
  std::ifstream file(path);
  if (!file)
    throw std::runtime_error("Cannot open LP file: " + path);

  ParsedLinearProgram lp;
  enum class Section { None, Objective, Constraints, Bounds };
  Section section = Section::None;
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

int main() {
  std::string file_path;
  // #ifdef 是 C/C++ 预处理器（Preprocessor）指令，_WIN32 为宏
#ifdef _WIN32
  file_path = "D:/chenzhen/CppOR/linear_programming/test_sets/afiro.lp";
#endif
  file_path = "/Users/zhenchen/CLionProjects/CppOR/linear_programming/afiro.lp";
  auto problem = parseLpFileData(file_path);
}