/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 11/08/2025, 23:42
 * Description:
 *
 */

#ifndef TETRIS_H
#define TETRIS_H

#include "Tetris.h"
#if __APPLE__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include "Piece.h"

#include <array>
#include <random>

// 一块方砖的大小
constexpr int cell_size = 30;
constexpr int game_width_block_num = 10;
constexpr int game_height_block_num = 20;
constexpr int score_width_block_num = game_width_block_num / 2;
constexpr int game_width = game_width_block_num * cell_size;
constexpr int game_height = game_height_block_num * cell_size;
constexpr int score_width = cell_size * score_width_block_num;
constexpr int total_width = game_width + score_width;

// 背景与线条颜色
constexpr auto background_color = SDL_Color{30, 30, 30, 255}; // RGB 深灰色
constexpr auto board_line_color = SDL_Color{80, 80, 80, 255}; // 浅灰色线条
constexpr auto block_line_color = SDL_Color{10, 10, 10, 100};

// 定义所有形状
const std::vector<std::vector<std::vector<int>>> SHAPES = {
    // 每个小列表是“行”，
    // 里面的数字代表“列”，
    // 1 是方块实际存在的位置0, 是空白格
    {{1, 1, 1, 1}},         // |
    {{1, 0, 0}, {1, 1, 1}}, // L
    {{0, 0, 1}, {1, 1, 1}}, // 反L
    {{1, 1}, {1, 1}},       // 正方形
    {{0, 1, 1}, {1, 1, 0}}, // 反Z形(S形)
    {{1, 1, 0}, {0, 1, 1}}, // Z形
    {{0, 1, 0}, {1, 1, 1}}  // T形
};

class Tetris {
  std::array<std::array<bool, game_width_block_num>, game_height_block_num> position_taken = {};
public:
  Tetris() = default;
  static const std::vector<std::vector<int>> &getRandomShape();
  static void set_background_color(SDL_Renderer *renderer, SDL_Color color);
  static void draw_grid(SDL_Renderer *renderer, SDL_Color color);
  void run();
  static void drawCell(SDL_Renderer *renderer, int x, int y, const SDL_Color &color);
  static void drawPiece(SDL_Renderer *renderer, const Piece &piece);
  bool validPosition(const Piece &piece) const;
};

#endif // TETRIS_H
