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
  SDL_Color RED{};
  SDL_Color GREEN{};
  SDL_Color BLUE{};
  SDL_Color WHITE{};
  SDL_Color YELLOW{};
  SDL_Color CYAN{};
  SDL_Color MAGENTA{};
  SDL_Color ORANGE{};
  // SDL_Color PURPLE{};

public:
  Color() {
    RED = {255, 0, 0, 255};
    GREEN = {0, 255, 0, 255};
    BLUE = {0, 0, 255, 255};
    WHITE = {255, 255, 255, 255};
    YELLOW = {255, 255, 0, 255};
    CYAN = {0, 255, 255, 255};
    MAGENTA = {255, 0, 255, 255};
    ORANGE = {255, 165, 0, 255};
    // PURPLE = {128, 0, 128, 255};
  };

  // 第一个 const 表示函数返回的是一个常量引用（const SDL_Color&）
  // 不能通过这个引用去修改返回的 SDL_Color 对象的值
  [[nodiscard]] const SDL_Color &getRandomColor() const;
  static SDL_Color adjustColor(SDL_Color color, float factor);
};

#endif // COLOR_H
