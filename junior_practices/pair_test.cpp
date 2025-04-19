/*
 * Created by Zhen Chen on 2025/4/4.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */
#include <iostream>
#include <utility> // For std::pair
#include <vector>

// template definition for product
template <typename T1, typename T2>
std::vector<std::pair<T1, T2>> product(const std::vector<T1> &a, const std::vector<T2> &b) {
  std::vector<std::pair<T1, T2>> result;
  for (const auto &i : a) {
    for (const auto &j : b) {
      result.emplace_back(i, j); // Create a pair and add it to the result
    }
  }
  return result;
}

int main() {
  // Example 1: product of two vectors with different types
  std::vector<std::vector<int>> a = {{1, 2, 3}};
  std::vector<std::string> b = {"A", "B", "C"};

  // Call product function
  auto result = product(a[0], b);

  // Print the result
  std::cout << "Product of a and b:" << std::endl;
  for (const auto &pair : result) {
    std::cout << "(" << pair.first << ", " << pair.second << ")" << std::endl;
  }

  // Example 2: product of two vectors with the same type
  std::vector<double> x = {1.1, 2.2};
  std::vector<double> y = {3.3, 4.4};

  auto result2 = product(x, y);

  std::cout << "Product of x and y:" << std::endl;
  for (const auto &pair : result2) {
    std::cout << "(" << pair.first << ", " << pair.second << ")" << std::endl;
  }

  return 0;
}
