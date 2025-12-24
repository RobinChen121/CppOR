/*
 * Created by Zhen Chen on 2025/12/24.
 * Email: chen.zhen5526@gmail.com
 * Description: 40 periods running time is 1.12s, only 0.05s faster than c++.
 *
 *
 */

#include "uthash.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ================== 参数设置 ================== */

#define T 40               // 时间阶段数
#define MAX_INVENTORY 150  // 最大库存
#define DEMAND_LAMBDA 20.0 // 泊松均值

#define C_ORDER 1.0     // 单位订购成本
#define C_HOLDING 2.0   // 单位持有成本
#define C_SHORTAGE 10.0 // 单位缺货成本
#define FIXED_COST 0.0  // 固定订购成本

//#define DEMAND_CUTOFF 100 // 泊松截断（≈5λ）

/* ================== uthash 状态定义 ================== */

typedef struct { // state
  int t;
  int x;
} StateKey;

typedef struct { // state with hash
  StateKey key; // (t, x)
  double value; // V(t,x)
  UT_hash_handle hh; // hash table handle
} ValueEntry;

ValueEntry *Vmemo = NULL;

/* ================== 泊松 PMF ================== */
double poissonPMF(int k) {
  if (k < 0)
    return 0.0;
  return exp(-DEMAND_LAMBDA + k * log(DEMAND_LAMBDA) - lgamma(k + 1));
}

/* ================== 前向声明 ================== */
double V(int t, int x);

/* ================== 单阶段期望成本 ================== */

double expected_cost(int t, int s) {
  double cost = 0.0;

  for (int d = 3; d <= 37; ++d) {
    double prob = poissonPMF(d) / (2*0.9999 - 1);
    int next_x = s - d;

    double stage_cost =
        C_HOLDING * (next_x > 0 ? next_x : 0) + C_SHORTAGE * (next_x < 0 ? -next_x : 0);

    cost += prob * (stage_cost + V(t + 1, next_x));
  }
  return cost;
}

/* ================== Bellman + memoization ================== */

double V(int t, int x) {
  /* 终止条件 */
  if (t == T)
    return 0.0;

  /* 库存截断（安全） */
  if (x > MAX_INVENTORY)
    x = MAX_INVENTORY;
  if (x < -MAX_INVENTORY)
    x = -MAX_INVENTORY;

  /* 查 memo */
  StateKey key = {t, x};
  ValueEntry *entry;

  HASH_FIND(hh, Vmemo, &key, sizeof(StateKey), entry);
  if (entry)
    return entry->value;

  double best = DBL_MAX;
  int maxOrder = MAX_INVENTORY - (x > 0 ? x : 0);
  for (int q = 0; q <= maxOrder; ++q) {
    double orderCost = (q > 0) ? FIXED_COST + C_ORDER * q : 0.0;
    int s = x + q;
    double val = orderCost + expected_cost(t, s); // recursion in the expected_cost function
    if (val < best)
      best = val;
  }

  /* 写入 memo */
  entry = (ValueEntry *)malloc(sizeof(ValueEntry));
  entry->key = key;
  entry->value = best;
  HASH_ADD(hh, Vmemo, key, sizeof(StateKey), entry);

  return best;
}

/* ================== 主函数 ================== */

int main() {
  struct timespec start, end;

  /* 开始计时 */
  clock_gettime(CLOCK_MONOTONIC, &start);
  double v0 = V(0, 0);
  /* 结束计时 */
  clock_gettime(CLOCK_MONOTONIC, &end);

  const double elapsed_ms =
      (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e6;

  printf("Optimal expected cost V(0,0) = %.6f\n", v0);
  printf("Number of memoized states = %u\n", HASH_COUNT(Vmemo));
  printf("DP computation time = %.3f ms\n", elapsed_ms);

  /* 释放 hash 表 */
  ValueEntry *cur, *tmp;
  HASH_ITER(hh, Vmemo, cur, tmp) {
    HASH_DEL(Vmemo, cur);
    free(cur);
  }

  return 0;
}
