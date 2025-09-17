#include <iostream>
#include <vector>

int main() {
  std::vector arr = {1, 2, 3};
  std::cout << "This is a test" << std::endl;

  arr.at(1) = 43; // 故意越界
  std::cout << "This will not be printed if ASan catches the error" << std::endl;

  return 0;
}
