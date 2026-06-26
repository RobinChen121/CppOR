/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 21/06/2026, 17:30
 * Description:
 *
 */
#include "Highs.h"
#include <filesystem>
#include <string>

int main() {
  // std::cout << "Current path is: " << std::filesystem::current_path() << std::endl;

  Highs highs;

  std::string file_path;
  // #ifdef 是 C/C++ 预处理器（Preprocessor）指令，_WIN32 为宏
#ifdef _WIN32
  file_path = "D:/chenzhen/CppOR/linear_programming/test_sets/afiro.mps";
#endif

  highs.readModel(file_path);

  highs.run();
  // highs.writeModel("afiro.lp");
}