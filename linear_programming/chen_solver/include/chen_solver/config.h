/*
 * Created by Zhen Chen on 2026/8/19.
 * Email: chen.zhen5526@gmail.com
 * Description: Project-wide shared types, enums and aliases.
 *
 *
 */

#ifndef CHEN_SOLVER_CONFIG_H
#define CHEN_SOLVER_CONFIG_H

#include <cstdint> // 避免 int32_t / uint64_t / uint8_t 找不到

// 更加明确这些整型占用的字节数
// ChenInt is for indexing
using ChenInt = int32_t;
using VarId = uint64_t;
using ConId = uint64_t;

// 标准 enum 默认通常占 4 字节（int 或 32 bit）。指定为 uint8_t（无符号 8
// 位整数）后，每个枚举实例仅占用 1 个字节（8 bit）
enum class ObjSense : uint8_t {
  Minimize, // Minimize the objective function
  Maximize,
};

enum class VarType : uint8_t { Continuous, Integer, Binary };

// 给模型添加一个状态
enum class ModelStatus : uint8_t {
  Empty,
  Modified,
  Compiled,
};

#endif // CHEN_SOLVER_CONFIG_H
