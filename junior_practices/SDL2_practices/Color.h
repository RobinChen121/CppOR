/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 12/08/2025, 10:05
 * Description:
 *
 */

#ifndef COLOR_H
#define COLOR_H

#if __APPLE__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif


// 主要是方块和边框颜色
class Color {

public:
  // 所有颜色用 constexpr 定义


  // Color() {
  //   RED = {255, 99, 71, 255}; // 番茄红
  //   // RED = {255, 0, 0, 255};
  //   GREEN = {124, 252, 0, 255}; // 草绿色
  //   // GREEN = {0, 255, 0, 255};
  //   // BLUE = {0, 0, 255, 255};
  //   BLUE = {135, 206, 235, 255}; // 天蓝色
  //   WHITE = {255, 255, 255, 255};
  //   YELLOW = {255, 215, 0, 255}; // 金黄色
  //   // YELLOW = {255, 255, 0, 255};
  //   CYAN = {0, 255, 255, 255};
  //   GRAY = {128, 128, 128, 255};
  //   MAGENTA = {255, 0, 255, 255};
  //   ORANGE = {255, 165, 0, 255};
  //   PURPLE = {128, 0, 128, 255};
  //   BLACK = {0, 0, 0, 255};
  // };

  // 第一个 const 表示函数返回的是一个常量引用（const SDL_Color&）
  // 不能通过这个引用去修改返回的 SDL_Color 对象的值
  static const SDL_Color &getRandomColor();
  static SDL_Color adjustColor(SDL_Color color, double factor);
};

#endif // COLOR_H
