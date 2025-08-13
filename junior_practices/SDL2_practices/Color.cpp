/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 12/08/2025, 10:05
 * Description:
 *
 */

#include "Color.h"
#include <iostream>
#include <random>

const SDL_Color &Color::getRandomColor() const {
  // 把所有颜色放进数组里
  const SDL_Color colorArray[] = {
      RED,  GREEN,   BLUE,  WHITE, YELLOW,
      CYAN, MAGENTA, ORANGE
      // , PURPLE
  };
  // 颜色数量
  constexpr int colorCount = std::size(colorArray);
  // 生成随机索引
  std::random_device rd;
  std::mt19937 gen(rd());
  // 随机选形状
  std::uniform_int_distribution<> colorDist(0, colorCount - 1);
  const int colorIndex = colorDist(gen);
  const auto &color = colorArray[colorIndex];
  return color;
}

// darker: factor < 1.0 (e.g., 0.7 代表暗 30%)
// lighter: factor > 1.0 (e.g., 1.3 代表亮 30%)
SDL_Color Color::adjustColor(const SDL_Color color, const float factor) {
  SDL_Color newColor;
  newColor.r = static_cast<Uint8>(SDL_clamp(color.r * factor, 0, 255));
  newColor.g = static_cast<Uint8>(SDL_clamp(color.g * factor, 0, 255));
  newColor.b = static_cast<Uint8>(SDL_clamp(color.b * factor, 0, 255));
  newColor.a = color.a;
  return newColor;
}
