/*
 * Created by Zhen Chen on 2025/8/11.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */
#include <SDL2/SDL.h>
#include <iostream>
int main() {
  // 初始化 SDL
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
    return 1;
  }

  // 一块方砖的大小
  constexpr int block_size = 30;
  constexpr int game_width_block_num = 10;
  constexpr int game_height_block_num = 20;
  constexpr int score_width_block_num = game_width_block_num / 2;
  constexpr int game_width = game_width_block_num * block_size;
  constexpr int game_height = game_height_block_num * block_size;
  constexpr int score_width = block_size * score_width_block_num;
  constexpr int total_width = game_width + score_width;

  // 颜色
  constexpr auto background_color = SDL_Color{30, 30, 30, 255}; // RGB 深灰色
  constexpr auto board_line_color = SDL_Color{80, 80, 80, 255}; // 浅灰色线条
  constexpr auto block_line_color = SDL_Color{255, 255, 255, 50};

  // 创建窗口
  // SDL_CreateWindow() 返回的是一个 指向 SDL_Window 结构体的指针
  SDL_Window *window =
      SDL_CreateWindow("Dr Zhen Chen's tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       total_width, game_height, SDL_WINDOW_SHOWN);
  if (!window) {
    std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }

  // 创建渲染器
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  // 启用混合模式，支持透明度设置
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  if (!renderer) {
    std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  // 主循环
  bool running = true;
  SDL_Event event;
  while (running) {
    // 处理事件
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      }
    }

    // 设置背景颜色
    SDL_SetRenderDrawColor(renderer, background_color.r, background_color.g, background_color.b,
                           background_color.a);
    SDL_RenderClear(renderer); // SDL_RenderClear() 用来清屏，通常不支持半透明清屏。

    // 画白色竖线，分隔左右两部分
    for (int i = 0; i < game_width_block_num + 1; i++) {
      SDL_SetRenderDrawColor(renderer, board_line_color.r, board_line_color.g, board_line_color.b,
                             board_line_color.a);
      SDL_RenderDrawLine(renderer, i * block_size, 0, i * block_size, game_height);
    }
    // 画白色横线
    for (int i = 0; i < game_height_block_num + 1; i++) {
      SDL_SetRenderDrawColor(renderer, board_line_color.r, board_line_color.g, board_line_color.b,
                             board_line_color.a);
      SDL_RenderDrawLine(renderer, 0, i * block_size, game_width, i * block_size);
    }

    SDL_Rect rect;
    rect.x = 90;         // 方块左上角 x 坐标
    rect.y = 90;         // 方块左上角 y 坐标
    rect.w = block_size; // 方块宽度
    rect.h = block_size; // 方块高度
    // 设置填充颜色
    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
    // 画填充矩形（方块）
    SDL_RenderFillRect(renderer, &rect);
    // 再画边框
    SDL_SetRenderDrawColor(renderer, block_line_color.r, block_line_color.g, block_line_color.b,
                           block_line_color.a);
    SDL_RenderDrawRect(renderer, &rect);

    // TODO: 在这里绘制俄罗斯方块方块、星星等

    SDL_RenderPresent(renderer);
  }

  // 清理资源
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
