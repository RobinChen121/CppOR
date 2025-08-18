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
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#else
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#endif

#include "Piece.h"
#include <array>
#include <random>

// 一块方砖的大小
constexpr int cell_size = 30;
constexpr int board_width = 10; // 游戏区的方砖数
constexpr int board_height = 20;
constexpr float drop_speed = 2; // 每秒下落几格
constexpr int score_width = board_width / 2;
constexpr int game_width_accurate = board_width * cell_size;
constexpr int game_height_accurate = board_height * cell_size;
constexpr int score_width_accurate = cell_size * score_width;
constexpr int total_width_accurate = game_width_accurate + score_width_accurate;

constexpr SDL_Color RED = {204, 79, 57, 255};     // 番茄红暗20%
constexpr SDL_Color GREEN = {99, 202, 0, 255};    // 草绿色暗20%
constexpr SDL_Color BLUE = {108, 164, 188, 255};  // 天蓝色暗20%
constexpr SDL_Color WHITE = {204, 204, 204, 255}; // 白色暗20%
constexpr SDL_Color YELLOW = {204, 172, 0, 255};  // 金黄色暗20%
constexpr SDL_Color CYAN = {0, 204, 204, 255};    // 青色暗20%
constexpr SDL_Color GRAY = {102, 102, 102, 255};  // 灰色暗20%
constexpr SDL_Color PURPLE = {148, 68, 169, 255}; // 紫色暗20%

// constexpr SDL_Color RED = {255, 99, 71, 255};    // 番茄红
// constexpr SDL_Color GREEN = {124, 252, 0, 255};  // 草绿色
// constexpr SDL_Color BLUE = {135, 206, 235, 255}; // 天蓝色
// constexpr SDL_Color WHITE = {255, 255, 255, 255};
// constexpr SDL_Color YELLOW = {255, 215, 0, 255}; // 金黄色
// constexpr SDL_Color CYAN = {0, 255, 255, 255};
// constexpr SDL_Color GRAY = {128, 128, 128, 255};
// constexpr SDL_Color PURPLE = {186, 85, 211, 255};
constexpr SDL_Color colorArray[] = {RED, GREEN, BLUE, WHITE, YELLOW, CYAN, GRAY,
                                    // MAGENTA,
                                    // ORANGE
                                    PURPLE};
//   // RED = {255, 0, 0, 255};
//   // GREEN = {0, 255, 0, 255};
//   // BLUE = {0, 0, 255, 255};
//   // YELLOW = {255, 255, 0, 255};
// constexpr SDL_Color MAGENTA= {255, 0, 255, 255};
// constexpr SDL_Color ORANGE = {255, 165, 0, 255};
// constexpr SDL_Color PURPLE = {128, 0, 128, 255};
// constexpr SDL_Color BLACK  = {0, 0, 0, 255};

// 背景与线条颜色
constexpr auto background_color = SDL_Color{30, 30, 30, 255}; // RGB 深灰色
constexpr auto board_line_color = SDL_Color{80, 80, 80, 255}; // 浅灰色线条
constexpr auto block_line_color = SDL_Color{158, 158, 158, 255};

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
  SDL_Renderer *renderer{};
  SDL_Window *window{};

  std::array<std::array<SDL_Color, board_height>, board_width> board_colors = {};
  std::array<std::array<bool, board_height>, board_width> position_taken = {};
  std::vector<int> lines_to_remove = {};

  // 音乐
  Mix_Chunk *rotate_sound{};
  Mix_Chunk *clear_sound{};
  Mix_Chunk *fix_sound{};
  Mix_Music *background_music{};

  // 字体
  TTF_Font *font{};
  SDL_Color font_color{};

  int highest_score = 0;
  int score = 0;
  int line_num = 0;

  // 定时器
  Uint32 last_drop_time = 0;

public:
  Tetris() {
    setWindow();
    setRender();
    setMusic();
    setFont();
  };
  void setWindow();
  void setRender();
  void setMusic();
  void setFont();
  void setFontSize(int n);
  void renderTextAt(const std::string &text, int x, int y) const;
  void renderLabelAndValue(const std::string &label, int value, int x, int y, int padding) const;
  static int read_file();
  static const std::vector<std::vector<int>> &getRandomShape();
  void setBackgroundColor(SDL_Color color) const;
  void drawGrid(SDL_Color color) const;
  void run();
  static Piece generatePiece();
  void drawCell(int x, int y, const SDL_Color &color) const;
  void drawPiece(const Piece &piece) const;
  void drawBoard() const;
  [[nodiscard]] bool validPosition(const Piece &piece) const;
  void dropDown(const Piece &piece);
  void lockPiece(const Piece &piece);
  void checkFullLines();
  void flashLines() const;
  void removeFullLines();
  void pauseAndWaitSpace() const;
  void pauseAndQuit() const;
  static void play_chunk_sound(Mix_Chunk *sound);
  static SDL_Color getRandomColor();
  static SDL_Color adjustColor(SDL_Color color, double factor);
};

#endif // TETRIS_H
