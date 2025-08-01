#include <iostream>

int main() {
  std::cout << "Hello World!\n";
  int a[3] = {1, 2, 3};
  a[5] = 42; // 越界访问
  return 0;
}
