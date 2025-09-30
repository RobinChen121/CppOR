/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 30/09/2025, 10:26
 * Description: This is branch and bound method that is not similar to little (1963)
 *
 */

#include "tsp.h"
#include <algorithm>
#include <iostream>

int n = static_cast<int>(C.size());
int best_cost = INT_MAX;
std::vector<int> best_path;
constexpr int INF = INT_MAX / 4; // 避免加法溢出
int globalMinEdge = INF;

// 用最近邻贪心从0出发得到一个初始上界（upper lower_bound）
int greedy_upper_lower_bound(std::vector<int> &out_path) {
  std::vector<char> used(n, 0);
  int current = 0;
  used[current] = 1;
  out_path.clear();
  out_path.push_back(current); // 起点是 0
  int cost = 0;
  for (int step = 1; step < n; ++step) {
    int next = -1, min_edge = INF;
    for (int v = 0; v < n; ++v) {
      if (!used[v] && C[current][v] < min_edge) { // 找一个最紧的邻居
        min_edge = C[current][v];
        next = v;
      }
    }
    if (next == -1 || min_edge >= INF) {
      return INF; // 无可行的扩展（图不连通）
    }
    used[next] = 1;
    out_path.push_back(next);
    cost += min_edge;
    current = next;
  }
  // 回到起点 0
  if (C[current][0] >= INF)
    return INF;
  cost += C[current][0];
  out_path.push_back(0);
  return cost;
}

// 回溯 + 剪枝
void dfs(const int current, const int visited_count, const int visited_mask, const int cost,
         std::vector<int> &path) {
  // 剪枝条件：保守下界 = 当前代价 + globalMinEdge * (剩余城市数)
  const int remaining = n - visited_count;
  const long long lower_bound =
      static_cast<long long>(cost) + static_cast<long long>(globalMinEdge) * remaining;
  if (lower_bound >= best_cost) // best_cost 是当前最好成本，即一个上界
    return;

  if (visited_count == n) {
    // 回到起点
    if (C[current][0] < INF) {
      int total = cost + C[current][0];
      if (total < best_cost) {
        best_cost = total;
        best_path = path;
        best_path.push_back(0); // 回到起点
      }
    }
    return;
  }

  // 枚举下一个城市（可以按启发式排序，提高剪枝效率）
  // collect candidates
  std::vector<std::pair<int, int>> candidate; // (edge cost, v)
  candidate.reserve(n);
  for (int v = 0; v < n; ++v) {
    // 若 v 还没被访问过
    if ((visited_mask & (1 << v)) == 0 && C[current][v] < INF) {
      candidate.emplace_back(C[current][v], v);
    }
  }
  // 按边权从小到大遍历（优先探索更有希望的分支）
  // 默认排序：升序，先比 first，再比 second
  std::ranges::sort(candidate);

  for (auto &[fst, snd] : candidate) {
    const int w = fst;
    int v = snd;
    const int next_cost = cost + w;
    if (next_cost >= best_cost)
      continue;        // 基本剪枝
    path.push_back(v); // 进入城市 v
    // 下面的 | 操作把城市 v 标记为已访问，不会影响其他城市的状态
    dfs(v, visited_count + 1, visited_mask | (1 << v), next_cost,
        path);       // 递归探索以 v 为当前城市的路径
    path.pop_back(); // 退出城市 v，返回上一级,意思是从另一个分支开始
  }
}

// ReSharper disable once CppDFAConstantFunctionResult
int main() {
  // 计算当前最小边 globalMinEdge
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j)
      if (i != j && C[i][j] < globalMinEdge)
        globalMinEdge = C[i][j];

  // 如果 graph 不连通或 globalMinEdge 非法，则无法求解
  if (globalMinEdge >= INF) {
    std::cout << "图不可达或边权全部为 INF\n";
    return 0;
  }

  // 用贪心得到初始best_cost
  std::vector<int> greedy_path;
  int greedy = greedy_upper_lower_bound(greedy_path);
  if (greedy < best_cost) {
    best_cost = greedy;
    best_path = greedy_path;
  }

  // DFS 分支限界，从城市 0 开始
  std::vector<int> path;
  path.push_back(0);
  constexpr int start_mask = 1 << 0; // 从 0 到 1
  dfs(0, 1, start_mask, 0, path);

  if (best_cost >= INT_MAX / 2) {
    std::cout << "no feasible solution\n";
  } else {
    std::cout << "the length of the shortest path: " << best_cost << "\n";
    std::cout << "the path (starts from 0 and ends at 0)";
    for (size_t i = 0; i < best_path.size(); ++i) {
      std::cout << best_path[i];
      if (i + 1 < best_path.size())
        std::cout << " -> ";
    }
    std::cout << "\n";
  }

  return 0;
}