/*
 * Created by Zhen Chen on 2025/8/13.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */
#include <iostream>
int main() {
  std::vector arr = {1, 2, 3};
  std::cout << "this is a test" << std::endl;
  arr.at(10) = 42; // 明显越界
}