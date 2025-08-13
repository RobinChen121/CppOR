/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 12/08/2025, 13:42
 * Description:
 *
 */

#ifndef PIECE_H
#define PIECE_H

#include <SDL_pixels.h>
#include <random>
#include <utility>
#include <vector>

class Piece {

public:
  int x; // grid x
  int y; // grid y
  SDL_Color color;
  std::vector<std::vector<int>> shape;

  Piece(const int x, const int y, const SDL_Color color, std::vector<std::vector<int>> shape)
      : x(x), y(y), color(color), shape(std::move(shape)) {
  };

  void rotate(int times);
  auto rotateRandom() -> void;
};

#endif // PIECE_H
