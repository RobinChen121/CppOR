#include <iostream>
#include <vector>

int main() {
  std::vector arr = {1, 2, 3};
  std::cout << "This is a test" << std::endl;

  // 开启 asan 越界检查，asan 会有一个 Sanitizer 显示 warning，点击
  // 会显示具体越界的数组
  // constexpr double arr2[] = {1.0, 2, 3};
  // std::cout << arr2[3] << std::endl;

  // 使用指针访问，这会触发 ASan 的内存越界检查，而不是 C++ 异常
  const int *ptr = arr.data();
  std::cout << "Attempting to access out-of-bounds memory..." << std::endl;
  // 强制越界访问
  std::cout << ptr[10] << std::endl;

  // 这种会直接抛出异常，asan 不会介入
  arr.at(4) = 43; // 故意越界
  std::cout << "This will not be printed if ASan catches the error" << std::endl;

  return 0;
}
