/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 11/08/2025, 23:42
 * Description:
 *
 */

#include "Tetris.h"
#include "Piece.h"
#if __APPLE__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include "Color.h"

#include <iostream>

// 随机选择一个形状
const std::vector<std::vector<int>> &Tetris::getRandomShape() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  // 随机选形状
  std::uniform_int_distribution<> shapeDist(0, static_cast<int>(SHAPES.size()) - 1);
  const int shapeIndex = shapeDist(gen);
  const auto &shape = SHAPES[shapeIndex];
  return shape;
}

void Tetris::drawCell(SDL_Renderer *renderer, const int x, const int y, const SDL_Color &color) {
  const SDL_Rect rect = {x + 1, y + 1, cell_size - 2, cell_size - 2};
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderFillRect(renderer, &rect);
  const auto lighter = Color::adjustColor(color, 1.5);
  const auto darker = Color::adjustColor(color, 0.5);

  // 你还可以加边框或阴影，方便视觉区分
  SDL_SetRenderDrawColor(renderer, lighter.r, lighter.g, lighter.b, lighter.a);
  SDL_RenderDrawLine(renderer, x, y + cell_size, x, y);
  SDL_RenderDrawLine(renderer, x, y, x + cell_size, y);
  SDL_SetRenderDrawColor(renderer, darker.r, darker.g, darker.b, darker.a);
  SDL_RenderDrawLine(renderer, x + 1, y + cell_size - 1, x + cell_size - 1, y + cell_size - 1);
  SDL_RenderDrawLine(renderer, x + cell_size - 1, y + cell_size - 1, x + cell_size - 1, y + 1);
}

void Tetris::drawPiece(SDL_Renderer *renderer, const Piece &piece) {
  for (int i = 0; i < piece.shape.size(); i++) {
    for (int j = 0; j < piece.shape[i].size(); j++) {
      if (piece.shape[i][j] > 1e-1) {
        const int px = (piece.x + j) * cell_size;
        const int py = (piece.y + i) * cell_size;
        if (piece.y > 8)
          std::cout << " ";
        drawCell(renderer, px, py, piece.color);
      }
    }
  }
}

void Tetris::lockPiece(const Piece &piece) {
  for (int i = 0; i < piece.shape.size(); i++) {
    for (int j = 0; j < piece.shape[i].size(); j++) {
      if (piece.shape[i][j] > 1e-1) {
        position_taken[i][j] = true;
      }
    }
  }
}

bool Tetris::validPosition(const Piece &piece) const {
  for (int i = 0; i < piece.shape.size(); i++) {
    for (int j = 0; j < piece.shape[0].size(); j++) {
      if (piece.shape[i][j] > 1e-1) {
        const int x = piece.x + j;
        const int y = piece.y + i;
        if (x < 0 || x >= game_width_block_num || y < 0 || y >= game_height_block_num ||
            (position_taken[x][y] == true)) {
          return false;
        }
      }
    }
  }
  return true;
}

void Tetris::set_background_color(SDL_Renderer *renderer, const SDL_Color color) {
  // 设置背景颜色
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderClear(renderer); // SDL_RenderClear() 用来清屏，通常不支持半透明清屏。
}

void Tetris::draw_grid(SDL_Renderer *renderer, const SDL_Color color) {
  // 画白色竖线，分隔左右两部分
  for (int i = 0; i < game_width_block_num + 1; i++) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(renderer, i * cell_size, 0, i * cell_size, game_height);
  }
  // 画白色横线
  for (int i = 0; i < game_height_block_num + 1; i++) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(renderer, 0, i * cell_size, game_width, i * cell_size);
  }
}

void Tetris::run() {
  // 创建窗口
  // SDL_CreateWindow() 返回的是一个 指向 SDL_Window 结构体的指针
  SDL_Window *window =
      SDL_CreateWindow("Dr Zhen Chen's tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       total_width, game_height, SDL_WINDOW_SHOWN);
  if (!window) {
    std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
  }

  // 创建渲染器
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  // 启用混合模式，支持透明度设置
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  if (!renderer) {
    std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
  }

  const auto colors = Color{};
  const auto color = Color::adjustColor(colors.getRandomColor(), 0.8);
  const auto next_color = Color::adjustColor(colors.getRandomColor(), 0.8);
  const auto shape = getRandomShape();
  auto piece = Piece(game_width_block_num / 2 - 1, 0, color, shape);
  piece.rotateRandom();
  const auto next_shape = getRandomShape();
  auto next_piece = Piece(game_width_block_num / 2, 0, next_color, next_shape);
  next_piece.rotateRandom();

  // 定时器
  auto last_drop_time = SDL_GetTicks();
  auto dropInterval = 1000; // 每 1 秒下落一格
  // 主循环，每帧循环一次
  bool running = true;
  SDL_Event event;
  while (running) {
    // 处理事件
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      }
    }

    // 背景色，网格线
    set_background_color(renderer, background_color);
    draw_grid(renderer, board_line_color);
    // 第一个方块
    drawPiece(renderer, piece);

    // 下一个方块
    Uint32 now = SDL_GetTicks();
    if (now - last_drop_time > dropInterval) {
      // 创建一个新的piece对象，模拟向下移动
      Piece piece_drop = piece;
      piece_drop.y += 1;
      if (validPosition(piece_drop)) {
        // 合法，更新位置
        piece = piece_drop;
      } else {
        // 不合法，方块到底了，处理下一步，比如固定方块，产生新方块等
      }
      last_drop_time = now;
    }

    // 是 SDL2 渲染流程中的一个关键函数，用来把之前所有绘制操作显示到屏幕上。
    SDL_RenderPresent(renderer);
  }

  // 清理资源
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}
