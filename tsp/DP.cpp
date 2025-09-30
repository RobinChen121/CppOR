/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 29/09/2025, 21:24
 * Description: dynamic programming for TSP.
 *
 */

#include "tsp.h"
#include <iostream>

int main() {
  const int n = static_cast<int>(C.size());

  // size 遍历的 mask 是一个整数，我们把它当作 二进制位集合 来用。
  // 如果某一位是 1 → 表示这个城市已经被访问过；
  // 如果某一位是 0 → 表示这个城市还没访问。
  const int size = 1 << n;
  // 状态总数：2^(n) 个状态, 表示把二进制数 1 左移 n 位
  // dp[mask][u] = 从起点出发，走过集合 mask 表示的城市，最后停在城市 u 的最短路径长度
  std::vector dp(size, std::vector(n, INT_MAX));

  // 记录从哪个城市转移过来
  std::vector<std::vector<int>> parent(size, std::vector<int>(n, -1));

  // 初始状态：只访问起点城市
  // 1<<0，其实表示的是 00000001，即第 0 个城市被访问
  dp[1 << 0][0] = 0; // 从城市 0 出发，只访问了城市 0

  // 状态压缩DP
  // 用 二进制位 表示哪些城市已经被访问过，从右向左 10101 表示路线经过城市 0，2，4
  // 这种方式的动态规划才高效，因为位运算非常快
  for (int mask = 0; mask < size; mask++) { // 2^n 个状态
    for (int u = 0; u < n; u++) {           // n 个城市
      // 这个 & 是按位运算，
      // 如果 mask 的 第 u 位 是 0，那么结果 = 0
      // 如果 mask 的 第 u 位 是 1，那么结果 != 0
      if (!(mask & (1 << u))) // 表示 u 没有被访问过，此时不能作为出发点，访问其他城市 v
        continue;
      if (dp[mask][u] == INT_MAX)
        continue;
      for (int v = 0; v < n; v++) { // 这里有 o(n) 个计算
        if (mask & (1 << v))
          continue; // v 已经访问过
        if (C[u][v] == INT_MAX)
          continue; // 无法到达
        // 下面的 | 操作把城市 v 标记为已访问，不会影响其他城市的状态
        const int nextMask = mask | (1 << v);
        // 下面的就是 DP 的递推公式
        if (dp[mask][u] + C[u][v] < dp[nextMask][v]) {
          dp[nextMask][v] = dp[mask][u] + C[u][v];
          parent[nextMask][v] = u; // 记录前驱
        }
      }
    }
  }

  int ans = INT_MAX;
  int last_city = -1;
  // dp[size - 1][u] 表示在8个城市的从城市0出发，所有路径集合中，最后到达城市u的最短路
  for (int u = 0; u < n; u++) {
    if (dp[size - 1][u] != INT_MAX && C[u][0] != INT_MAX) {
      if (dp[size - 1][u] + C[u][0] < ans) {
        ans = dp[size - 1][u] + C[u][0];
        last_city = u;
      }
    }
  }
  std::cout << "最短路径长度: " << ans << std::endl;

  std::vector<int> path;
  int mask = size - 1, u = last_city;
  while (u != -1) {
    path.push_back(u);
    const int p = parent[mask][u];
    mask ^= (1 << u); // 把城市 u 从集合里移除
    u = p;
  }
  reverse(path.begin(), path.end());
  path.push_back(0);

  std::cout << "最短路径: ";
  for (const int x : path)
    std::cout << x << " ";
  std::cout << std::endl;

  return 0;
}