/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 11/08/2025, 23:42
 * Description: mac does not support sdl2 very well.
 *
 */

#include "Tetris.h"
#include "Piece.h"
#include <filesystem>
#if __APPLE__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#include <SDL_mixer.h>
#endif
#include <fstream>
#include <iostream>
#include <string>

void Tetris::setMusic() {
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    std::cerr << "Mix_OpenAudio Error: " << Mix_GetError() << "\n";
    SDL_Quit();
  } else {
    // 调整背景音乐音量（0 ~ 128）
    Mix_VolumeMusic(50);
    rotate_sound = Mix_LoadWAV("../assets/rotate.wav");
    clear_sound = Mix_LoadWAV("../assets/clear.wav");
    fix_sound = Mix_LoadWAV("../assets/fix.wav");
    background_music = Mix_LoadMUS("../assets/background.mp3");
    // 加载完音乐后才能播放
    Mix_PlayMusic(background_music, -1); // -1 表示无限循环
  }
}

void Tetris::setWindow() {
  // 创建窗口
  // SDL_CreateWindow() 返回的是一个 指向 SDL_Window 结构体的指针
  window = SDL_CreateWindow("My tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            total_width_accurate, game_height_accurate, SDL_WINDOW_SHOWN);
  if (!window) {
    std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
  }
}

void Tetris::setRender() {
  // 创建渲染器 - 为Mac提供更好的兼容性
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  // 如果硬件加速失败，尝试软件渲染器
  if (!renderer) {
    std::cout << "Hardware accelerated renderer failed, trying software renderer..." << std::endl;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  }

  if (!renderer) {
    std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
  }

  // 设置渲染器属性，减少Mac上的闪烁问题
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  // 创建窗口和 renderer 后，先 present 一次背景,不然会先黑屏
  setBackgroundColor(background_color);
  drawGrid(board_line_color);
  SDL_RenderPresent(renderer);
}

void Tetris::setFont() {
  // 必须先初始化字体程序
  if (TTF_Init() == -1) {
    std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
    return;
  }
  font = TTF_OpenFont("../assets/NotoSansSC-Regular.otf", 24); // 支持中文
  font_color = WHITE;
}

// n is font size
void Tetris::setFontSize(const int n) {
  font = TTF_OpenFont("../assets/NotoSansSC-Regular.otf", n); // 支持中文
}

// 渲染文本到屏幕指定位置并及时销毁
// 渲染文字并自动销毁纹理
void Tetris::renderTextAt(const std::string &text, const int x, const int y) const {
  SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text.c_str(), font_color);
  if (!surface) {
    SDL_Log("TTF_RenderUTF8_Blended error: %s", TTF_GetError());
    return;
  }

  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  if (!texture) {
    SDL_Log("SDL_CreateTextureFromSurface error: %s", SDL_GetError());
    return;
  }

  int text_width, text_height;
  SDL_QueryTexture(texture, nullptr, nullptr, &text_width, &text_height);
  const SDL_Rect dstRect = {x, y, text_width, text_height};
  SDL_RenderCopy(renderer, texture, nullptr, &dstRect);

  SDL_DestroyTexture(texture); // 及时释放纹理
}

// 渲染一行 "标签 + 数字"
void Tetris::renderLabelAndValue(const std::string &label, const int value, const int x,
                                 const int y, const int padding = 5) const {
  // 渲染标签
  SDL_Surface *surface_label = TTF_RenderUTF8_Blended(font, label.c_str(), font_color);
  SDL_Texture *tex_label = SDL_CreateTextureFromSurface(renderer, surface_label);
  int label_w, label_h;
  SDL_QueryTexture(tex_label, nullptr, nullptr, &label_w, &label_h);
  const SDL_Rect rect_label = {x, y, label_w, label_h};
  SDL_RenderCopy(renderer, tex_label, nullptr, &rect_label);
  SDL_DestroyTexture(tex_label);
  SDL_FreeSurface(surface_label);

  // 渲染数值
  const std::string value_str = std::to_string(value);
  SDL_Surface *surface_val = TTF_RenderUTF8_Blended(font, value_str.c_str(), font_color);
  SDL_Texture *tex_val = SDL_CreateTextureFromSurface(renderer, surface_val);
  int value_w, value_h;
  SDL_QueryTexture(tex_val, nullptr, nullptr, &value_w, &value_h);
  const SDL_Rect rect_val = {x + label_w + padding, y, value_w, value_h};
  SDL_RenderCopy(renderer, tex_val, nullptr, &rect_val);
  SDL_DestroyTexture(tex_val);
  SDL_FreeSurface(surface_val);
}

int Tetris::read_file() {
  std::ifstream inFile("../assets/high_score.txt"); // 打开分数文件
  std::string line;
  getline(inFile, line);
  return std::stoi(line);
}

SDL_Color Tetris::getRandomColor() {
  // 颜色数量
  constexpr int colorCount = std::size(colorArray);
  // 生成随机索引
  std::random_device rd;
  std::mt19937 gen(rd());
  // 随机选形状
  std::uniform_int_distribution<> colorDist(0, colorCount - 1);
  const int colorIndex = colorDist(gen);
  const auto color = colorArray[colorIndex];
  return color;
}

// darker: factor < 1.0 (e.g., 0.7 代表暗 30%)
// lighter: factor > 1.0 (e.g., 1.3 代表亮 30%)
SDL_Color Tetris::adjustColor(const SDL_Color color, const double factor) {
  SDL_Color newColor;
  newColor.r = static_cast<Uint8> SDL_clamp(((color.r * factor)), 0, 255);
  newColor.g = static_cast<Uint8> SDL_clamp(((color.g * factor)), 0, 255);
  newColor.b = static_cast<Uint8> SDL_clamp(((color.b * factor)), 0, 255);
  newColor.a = 255;
  return newColor;
}

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

void Tetris::drawCell(const int x, const int y, const SDL_Color &color) const {
  const SDL_Rect rect = {x + 1, y + 1, cell_size - 2, cell_size - 2};
  const auto color1 = adjustColor(color, 1);
  SDL_SetRenderDrawColor(renderer, color1.r, color1.g, color1.b, color1.a);
  // std::cout<<static_cast<int>(color1.a)<<std::endl;
  SDL_RenderFillRect(renderer, &rect);
  const auto lighter = adjustColor(color, 1.5);
  const auto darker = adjustColor(color, 0.5);

  // 你还可以加边框或阴影，方便视觉区分
  SDL_SetRenderDrawColor(renderer, lighter.r, lighter.g, lighter.b, lighter.a);
  // std::cout<<static_cast<int>(lighter.a)<<std::endl;
  SDL_RenderDrawLine(renderer, x, y + cell_size, x, y);
  SDL_RenderDrawLine(renderer, x, y, x + cell_size, y);
  SDL_SetRenderDrawColor(renderer, darker.r, darker.g, darker.b, darker.a);
  // std::cout<<static_cast<int>(darker.a)<<std::endl;
  SDL_RenderDrawLine(renderer, x + 1, y + cell_size - 1, x + cell_size - 1, y + cell_size - 1);
  SDL_RenderDrawLine(renderer, x + cell_size - 1, y + cell_size - 1, x + cell_size - 1, y + 1);
}

void Tetris::drawPiece(const Piece &piece) const {
  for (int i = 0; i < piece.shape.size(); i++) {
    for (int j = 0; j < piece.shape[i].size(); j++) {
      if (piece.shape[i][j] > 1e-1) {
        const int px = (piece.x + j) * cell_size;
        const int py = (piece.y + i) * cell_size;
        drawCell(px, py, piece.color);
      }
    }
  }
}

void Tetris::lockPiece(const Piece &piece) {

  for (int i = 0; i < piece.shape.size(); i++) {
    for (int j = 0; j < piece.shape[i].size(); j++) {
      if (piece.shape[i][j] > 1e-1) {
        const int board_x = piece.x + j;
        const int board_y = piece.y + i;
        position_taken[board_x][board_y] = true;
        board_colors[board_x][board_y] = piece.color;
      }
    }
  }
  checkFullLines();
  if (!lines_to_remove.empty())
    removeFullLines();
  else
    play_chunk_sound(fix_sound);
}

void Tetris::checkFullLines() {
  for (int y = board_height - 1; y >= 0; --y) {
    bool lineIsFull = true;
    for (int x = 0; x < board_width; ++x) {
      if (!position_taken[x][y]) {
        lineIsFull = false;
        break;
      }
    }
    if (lineIsFull) {
      lines_to_remove.push_back(y);
    }
  }
}

// flash is very difficult to make effect in mac
void Tetris::flashLines() const {
  constexpr int flash_count = 5;  // 闪烁次数
  constexpr int flash_delay = 80; // 增加延时，减少Mac上的雪花问题

  // 保存当前混合模式
  SDL_BlendMode currentBlendMode = SDL_BLENDMODE_NONE; // 设置默认值
  if (SDL_GetRenderDrawBlendMode(renderer, &currentBlendMode) != 0) {
    // 如果获取失败，currentBlendMode 保持默认值 SDL_BLENDMODE_NONE
    // 如果获取成功，currentBlendMode 将包含实际的当前混合模式
  }

  // 设置适合闪烁的混合模式
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  // 先绘制一次完整的背景和已锁定方块
  SDL_SetRenderDrawColor(renderer, background_color.r, background_color.g, background_color.b, 255);
  SDL_RenderClear(renderer); // 这行命令的主要作用是清屏

  // 重绘网格
  drawGrid(board_line_color);

  // 重绘所有已放置的方块
  for (int row = 0; row < board_height; ++row) {
    for (int col = 0; col < board_width; ++col) {
      if (position_taken[col][row]) {
        drawCell(col * cell_size, row * cell_size, board_colors[col][row]);
      }
    }
  }

  for (int i = 0; i < flash_count; ++i) {
    // 只更新需要闪烁的行，而不是重绘整个屏幕
    if (i % 2 == 1) { // 显示状态
      for (const int line : lines_to_remove) {
        for (int x = 0; x < board_width; ++x) {
          drawCell(x * cell_size, line * cell_size, WHITE);
        }
      }
    } else {
      for (const int line : lines_to_remove) {
        for (int x = 0; x < board_width; ++x) {
          drawCell(x * cell_size, line * cell_size, board_colors[x][line]);
        }
      }
    }

    SDL_RenderPresent(renderer);
    SDL_Delay(flash_delay);
  }

  // 恢复原始混合模式
  SDL_SetRenderDrawBlendMode(renderer, currentBlendMode);

  SDL_Delay(100); // 爆炸完延迟0.1s
}

void Tetris::removeFullLines() {
  int lines_remove_num = static_cast<int>(lines_to_remove.size());
  line_num += lines_remove_num;
  switch (lines_remove_num) {
  case 1:
    score += lines_remove_num * 100;
    break;
  case 2:
    score += lines_remove_num * 150;
    break;
  case 3:
    score += lines_remove_num * 200;
    break;
  case 4:
    score += lines_remove_num * 250;
    break;
  case 5:
    score += lines_remove_num * 300;
    break;
  default:
    score += lines_remove_num * 300;
  }
  flashLines();

  // for (const auto line : lines_to_remove) {
  //   for (int col = 0; col < board_width; ++col) {
  //     auto color = adjustColor(board_colors[col][line], 0.8);
  //     drawCell(col * cell_size, line * cell_size, color);
  //   }
  // }
  // SDL_RenderPresent(renderer);

  int k = 0;
  while (lines_remove_num > 0) {
    int max_line = lines_to_remove[k] + k;
    for (int row = max_line; row > 0; --row) {
      for (int col = 0; col < board_width; ++col) {
        position_taken[col][row] = position_taken[col][row - 1];
        board_colors[col][row] = board_colors[col][row - 1];
      }
    }
    lines_remove_num--;
    k++;
    play_chunk_sound(clear_sound);
  }
  lines_to_remove.clear();
}

void Tetris::pauseAndWaitSpace() const {
  // 绘制半透明背景
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150); // 半透明黑
  SDL_Rect overlay = {0, 0, total_width_accurate, game_height_accurate};
  SDL_RenderFillRect(renderer, &overlay);

  // 绘制提示框
  SDL_SetRenderDrawColor(renderer, background_color.r, background_color.g, background_color.b, 255);
  SDL_Rect box = {cell_size * 2, game_height_accurate / 3, total_width_accurate / 2,
                  game_height_accurate / 5};
  SDL_RenderFillRect(renderer, &box);

  // 在提示框上渲染文字
  renderTextAt("Game Paused", box.x + cell_size * 2, box.y + cell_size);
  renderTextAt("Press Space again to continue", box.x + 10, box.y + cell_size * 2);

  // 显示暂停提示
  SDL_RenderPresent(renderer);

  // 等待空格键
  // 第一步：清空事件队列，避免残留
  SDL_FlushEvent(SDL_KEYUP);
  SDL_Event e;
  bool waiting = true;
  while (waiting) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        exit(0); // 退出游戏
      }
      if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_SPACE) {
        waiting = false; // 空格继续
        // 继续播放已暂停的背景音乐
        Mix_ResumeMusic();
      }
    }
    SDL_Delay(5);
  }
}

void Tetris::pauseAndQuit() const {
  // 绘制半透明背景
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150); // 半透明黑
  SDL_Rect overlay = {0, 0, total_width_accurate, game_height_accurate};
  SDL_RenderFillRect(renderer, &overlay);

  // 绘制提示框
  SDL_SetRenderDrawColor(renderer, background_color.r, background_color.g, background_color.b, 255);
  SDL_Rect box = {cell_size * 2, game_height_accurate / 3, total_width_accurate / 2,
                  game_height_accurate / 5};
  SDL_RenderFillRect(renderer, &box);

  // 在提示框上渲染文字
  renderTextAt("Game Over", static_cast<int>(box.x + cell_size * 2.5), box.y + cell_size);
  renderTextAt("Press any key to quit", static_cast<int>(box.x + cell_size * 1.5),
               box.y + cell_size * 2);

  // 显示暂停提示
  SDL_RenderPresent(renderer);

  // 改良的等待循环
  SDL_Event event;
  bool waiting = true;
  while (waiting) {
    SDL_PumpEvents(); // <- 关键：强制更新事件队列

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN ||
          event.type == SDL_MOUSEBUTTONDOWN) {
        waiting = false;
      }
    }
    SDL_Delay(10); // 给 CPU 休息，避免占满
  }
}

void Tetris::play_chunk_sound(Mix_Chunk *sound) {
  // -1 表示自动选择一个空闲的声道（channel）来播放音效
  Mix_VolumeChunk(sound, 50);
  Mix_PlayChannel(-1, sound, 0); // 播放音效
}

void Tetris::drawBoard() const {
  for (int y = 0; y < board_height; ++y) {
    for (int x = 0; x < board_width; ++x) {
      if (position_taken[x][y]) {
        auto dark_color = adjustColor(board_colors[x][y], 0.8);
        drawCell(x * cell_size, y * cell_size, dark_color);
      }
    }
  }
}

Piece Tetris::generatePiece() {
  const auto color = getRandomColor();
  const auto shape = getRandomShape();
  auto piece = Piece(board_width / 2 - 1, 0, color, shape);
  piece.rotateRandom();
  return piece;
}

bool Tetris::validPosition(const Piece &piece) const {
  for (int i = 0; i < piece.shape.size(); i++) {
    for (int j = 0; j < piece.shape[0].size(); j++) {
      if (piece.shape[i][j] > 1e-1) {
        const int x = piece.x + j;
        const int y = piece.y + i;
        if (x < 0 || x >= board_width || y < 0 || y >= board_height ||
            (y > 0 && position_taken[x][y] == true)) {
          return false;
        }
      }
    }
  }
  return true;
}

void Tetris::setBackgroundColor(const SDL_Color color) const {
  // 设置背景颜色
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderClear(renderer); // SDL_RenderClear() 用来清屏，通常不支持半透明清屏。
}

void Tetris::drawGrid(const SDL_Color color) const {
  // 画白色竖线，分隔左右两部分
  for (int i = 0; i < board_width + 1; i++) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(renderer, i * cell_size, 0, i * cell_size, game_height_accurate);
  }
  // 画白色横线
  for (int i = 0; i < board_height + 1; i++) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(renderer, 0, i * cell_size, game_width_accurate, i * cell_size);
  }
}

void Tetris::run() {
  // 加载音频
  // setMusic();
  auto piece = generatePiece();
  // 确定下一个方块
  auto next_piece = generatePiece();
  // 定时器
  last_drop_time = SDL_GetTicks();
  auto dropInterval = 1000 / drop_speed; // 每 1 秒下落一格
  highest_score = read_file();

  bool running = true;
  SDL_Event event;
  bool paused = false; // 暂停状态
  while (running) {

    // 2. 清屏（每帧开始时）
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255); // 设置背景色，例如黑色
    SDL_RenderClear(renderer);

    // 处理事件
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      }
      // 按下键
      if (event.type == SDL_KEYDOWN) {
        const SDL_Keycode key = event.key.keysym.sym;
        if (key == SDLK_LEFT) {
          piece.x -= 1;
          if (!validPosition(piece))
            piece.x += 1;
        }
        if (key == SDLK_RIGHT) {
          piece.x += 1;
          if (!validPosition(piece))
            piece.x -= 1;
        }
        if (key == SDLK_DOWN) {
          piece.y += 1;
          if (!validPosition(piece))
            piece.y -= 1;
          last_drop_time = SDL_GetTicks();
        }
        if (key == SDLK_UP) {
          piece.rotate(1);
          if (!validPosition(piece))
            piece.rotate(3);
          play_chunk_sound(rotate_sound);
        }
        if (key == SDLK_SPACE) {
          paused = !paused; // 按空格切换暂停/继续
          pauseAndWaitSpace();
          if (paused) {
            Mix_PauseMusic(); // 暂停背景音乐
          } else {
            Mix_ResumeMusic(); // 恢复背景音乐
          }
        }
      }
    }

    // 2. 下落逻辑
    if (!paused) {
      Uint32 now = SDL_GetTicks();
      if (now - last_drop_time > static_cast<Uint32>(dropInterval)) {
        piece.y += 1;
        piece.first_appear = false;
        if (!validPosition(piece)) {
          piece.y -= 1;
          lockPiece(piece);
          piece = next_piece;
          next_piece = generatePiece();
        }
        last_drop_time = now;
      }

      // 每帧开始
      // 画背景色，网格线
      setBackgroundColor(background_color);
      drawGrid(board_line_color);

      // 保证新的方块从中间出来
      std::vector<std::vector<int>> special_shape = {{1, 1, 1, 1}};
      if (piece.shape == special_shape && piece.first_appear)
        piece.x = board_width / 2 - 2;
      drawPiece(piece); // 渲染正在下落的方块
      drawBoard();      // 渲染固定的方块

      // 显示文字
      // auto font_color = WHITE;
      std::string next_text = "NEXT";
      renderTextAt(next_text, static_cast<int>(game_width_accurate + cell_size * 1.5), 0);

      setFontSize(15);
      renderLabelAndValue("Highest:", highest_score, game_width_accurate + cell_size / 4,
                          cell_size * 8);
      renderLabelAndValue("Score:", score, game_width_accurate + cell_size / 4, cell_size * 9);
      renderLabelAndValue("Lines:", line_num, game_width_accurate + cell_size / 4, cell_size * 10);

      renderTextAt(u8"↑: rotate left 90°", game_width_accurate + cell_size / 4, cell_size * 15);
      renderTextAt("[SPACE] to pause", game_width_accurate + cell_size / 4, cell_size * 16);

      if (score > highest_score) {
        highest_score = score;
        std::ofstream outFile("../assets/high_score.txt");
        outFile << highest_score;
        outFile.close();
      }

      // 提示下一个
      auto hint_piece = next_piece;
      hint_piece.x = board_width + 1;
      hint_piece.y = 2;
      drawPiece(hint_piece);

      if (piece.first_appear && !validPosition(piece)) {
        pauseAndQuit();
        running = false;
      }

      // 是 SDL2 渲染流程中的一个关键函数，用来把之前所有绘制操作显示到屏幕上
      SDL_RenderPresent(renderer);
      SDL_Delay(30); // 让程序 暂停 16 毫秒，再继续执行下一行代码。
    }
  }

  // 清理资源
  Mix_FreeMusic(background_music);
  Mix_FreeChunk(rotate_sound);
  Mix_FreeChunk(clear_sound);
  Mix_FreeChunk(fix_sound);
  Mix_CloseAudio(); // 关闭音频
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}
