/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 24/06/2026, 22:59
 * Description:
 *
 */

#include "read_file.h"
#include <string>

int main() {
  std::string file_path;
  // #ifdef 是 C/C++ 预处理器（Preprocessor）指令，_WIN32 为宏
#ifdef _WIN32
  file_path = "D:/chenzhen/CppOR/linear_programming/test_sets/afiro.lp";
#endif

  auto problem = parseLpFileData(file_path);
}