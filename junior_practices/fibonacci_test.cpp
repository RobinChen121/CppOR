//
// Created by Zhen Chen on 2025/2/22.
//

#include "fibonacci_test.h"

#include <iostream>
#include <unordered_map>
#include <chrono>
using namespace std;

// 使用 unordered_map 缓存 Fibonacci 计算结果
unordered_map<int, cpp_int> cache;

// Fibonacci 函数
cpp_int fibonacci(int n) {
     // 基本情况
     if (n == 0) return 0;
     if (n == 1) return 1;

     // 检查缓存中是否已经计算过
     if (cache.find(n) != cache.end()) {
         return cache[n];
     }

     // 递归计算并缓存结果

     cpp_int result = fibonacci(n - 1) + fibonacci(n - 2);
     cache[n] = result;
     return result;
 }
