#include "junior_practices/fibonacci_test.h"
#include <iostream>
#include <chrono>

// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or
// click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
int main() {
    // TIP Press <shortcut actionId="RenameElement"/> when your caret is at the
    // <b>lang</b> variable name to see how CLion can help you rename it.
    // auto lang = "C++";
    // std::cout << "Hello and welcome to " << lang << "!\n";
    //
    // for (int i = 1; i <= 5; i++) {
    //     // TIP Press <shortcut actionId="Debug"/> to start debugging your code.
    //     // We have set one <icon src="AllIcons.Debugger.Db_set_breakpoint"/>
    //     // breakpoint for you, but you can always add more by pressing
    //     // <shortcut actionId="ToggleLineBreakpoint"/>.
    //     std::cout << "i = " << i << std::endl;
    // }

    int n = 50000;
    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "Fibonacci(" << n << ") = " << fibonacci(n) << std::endl;
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "running time is " << duration << std::endl;
    return 0;
}

// TIP See CLion help at <a
// href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>.
//  Also, you can try interactive lessons for CLion by selecting
//  'Help | Learn IDE Features' from the main menu.


// #include <iostream>
// #include <chrono>
// #include <unordered_map>
// #include <boost/multiprecision/cpp_int.hpp>  // 包含 cpp_int
//
// using namespace boost::multiprecision;
// using namespace std;
//
// // 使用 unordered_map 缓存 Fibonacci 计算结果
// unordered_map<int, cpp_int> cache;
//
// // Fibonacci 函数
// cpp_int fibonacci(int n) {
//     // 基本情况
//     if (n == 0) return 0;
//     if (n == 1) return 1;
//
//     // 检查缓存中是否已经计算过
//     if (cache.find(n) != cache.end()) {
//         return cache[n];
//     }
//
//     // 递归计算并缓存结果
//
//     cpp_int result = fibonacci(n - 1) + fibonacci(n - 2);
//     cache[n] = result;
//     return result;
// }
//
// int main() {
//     int n = 2500;
//
//     // 计算并输出结果
//     auto start = std::chrono::high_resolution_clock::now();
//     cout << "Fibonacci(" << n << ") = " << fibonacci(n) << endl;
//     auto end = std::chrono::high_resolution_clock::now();
//     std::chrono::duration<double> duration = end - start;
//     std::cout << "running time is " << duration << std::endl;
//
//     return 0;
// }

// #include <iostream>
// #include <chrono>
// using namespace std;
//
// // Recursive Fibonacci
// long long fibRecursive(int n) {
//     if (n <= 1) return n;
//     return fibRecursive(n - 1) + fibRecursive(n - 2);
// }
//
// // Iterative Fibonacci
// long long fibIterative(int n) {
//     long long a = 0, b = 1;
//     for (int i = 0; i < n; i++) {
//         long long temp = a + b;
//         a = b;
//         b = temp;
//     }
//     return a;
// }
//
// int main() {
//     auto start = chrono::high_resolution_clock::now();
//     fibRecursive(40);
//     auto end = chrono::high_resolution_clock::now();
//     chrono::duration<double> duration = end - start;
//     cout << "C++ recursive Fibonacci time: " << duration.count() << " seconds" << endl;
//
//     start = chrono::high_resolution_clock::now();
//     fibIterative(30);
//     end = chrono::high_resolution_clock::now();
//     duration = end - start;
//     cout << "C++ iterative Fibonacci time: " << duration.count() << " seconds" << endl;
//
//     return 0;
// }