/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 30/09/2025, 21:34
 * Description:
 *
 */
#include "tsp.h"
#include <iostream>
#include <queue>

#include <limits>
#include <vector>
using namespace std;

constexpr int INF = 1e9;

struct Node {
  vector<vector<int>> currentMatrix; // current length matrix for the node
  int lowerBound{};                  // 当前节点的下界
  int costSoFar{};                   // 到目前为止的真实花费
  int vertex{};                      // 当前城市
  int level{};                       // 已访问城市数
  vector<int> path;
};

/**
 *
 * @param mat length matrix
 * @return total reduced cost
 */
int reduceMatrix(vector<vector<int>> &mat) {
  const int n = static_cast<int>(mat.size());
  int reductionCost = 0;

  // 行约简
  for (int i = 0; i < n; i++) {
    int rowMin = INF;
    for (int j = 0; j < n; j++)
      rowMin = min(rowMin, mat[i][j]);
    if (rowMin != INF && rowMin > 0) {
      reductionCost += rowMin;
      for (int j = 0; j < n; j++)
        if (mat[i][j] != INF)
          mat[i][j] -= rowMin;
    }
  }

  // 列约简
  for (int j = 0; j < mat.size(); j++) {
    int colMin = INF;
    for (auto &i : mat)
      colMin = min(colMin, i[j]);
    if (colMin != INF && colMin > 0) {
      reductionCost += colMin;
      for (auto &i : mat)
        if (i[j] != INF)
          i[j] -= colMin;
    }
  }

  return reductionCost;
}

// 用指针返回主要是性能考虑和节点管理方便
/**
 *
 * @param parent
 * @param i 起点城市
 * @param j 即将到达的城市
 * @param originalMatrix
 * @return
 */
Node *createChild(const Node &parent, const int i, const int j,
                  const vector<vector<int>> &originalMatrix) {
  const int n = static_cast<int>(parent.currentMatrix.size());
  const auto child = new Node; // 创建一个 Node 指针
  child->path = parent.path;
  child->path.push_back(j);
  child->level = parent.level + 1;
  child->vertex = j;

  child->currentMatrix = parent.currentMatrix;
  for (int k = 0; k < n; k++) {
    child->currentMatrix[i][k] = INF;
    child->currentMatrix[k][j] = INF;
  }
  child->currentMatrix[j][0] = INF; // 避免形成子环

  const int extraReduction = reduceMatrix(child->currentMatrix);
  child->costSoFar = parent.costSoFar + originalMatrix[i][j];
  child->lowerBound = child->costSoFar + extraReduction;

  return child;
}

struct comp {
  bool operator()(const Node *lhs, const Node *rhs) { return lhs->lowerBound > rhs->lowerBound; }
};

void solveTSP(const vector<vector<int>> &C) {
  const int n = static_cast<int>(C.size());
  vector<vector<int>> reduced = C;
  const int rootReduction = reduceMatrix(reduced);

  Node *root = new Node;
  root->path.push_back(0);
  root->vertex = 0;
  root->level = 0;
  root->currentMatrix = reduced;
  root->costSoFar = 0;
  root->lowerBound = rootReduction;

  priority_queue<Node *, vector<Node *>, comp> pq;
  pq.push(root);

  int bestCost = INF;
  vector<int> bestPath;

  while (!pq.empty()) {
    Node *minNode = pq.top();
    pq.pop();
    int i = minNode->vertex;

    if (minNode->level == n - 1) {
      int finalCost = minNode->costSoFar + C[i][0];
      if (finalCost < bestCost) {
        bestCost = finalCost;
        bestPath = minNode->path;
        bestPath.push_back(0);
      }
      delete minNode;
      continue;
    }

    for (int j = 0; j < n; j++) {
      if (minNode->currentMatrix[i][j] != INF) {
        Node *child = createChild(*minNode, i, j, C);
        if (child->lowerBound < bestCost) {
          pq.push(child);
        } else {
          delete child;
        }
      }
    }
    delete minNode;
  }

  cout << "最优路径: ";
  for (int v : bestPath)
    cout << v << " ";
  cout << "\n最小花费: " << bestCost << endl;
}

int main() {
  // const vector<vector<int>> C = {{0, 5, 21, 13, 6, 15, 18, 20},   {5, 0, 16, 18, 7, 12, 19, 17},
  //                                {21, 16, 0, 33, 16, 7, 17, 11},  {13, 18, 33, 0, 17, 26, 16,
  //                                29}, {6, 7, 16, 17, 0, 9, 12, 14},    {15, 12, 7, 26, 9, 0, 10,
  //                                5}, {18, 19, 17, 16, 12, 10, 0, 13}, {20, 17, 11, 29, 14, 5, 13,
  //                                0}};

  solveTSP(C);
  return 0;
}