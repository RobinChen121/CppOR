/*
 * Created by Zhen Chen on 2025/8/11.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */
#if __APPLE__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include "Tetris.h"

#include <iostream>


int main(int argc, char *argv[]) {
  // 初始化 SDL
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
    return 1;
  }

  Tetris game;
  game.run();

  SDL_Quit();
  return 0;
}
