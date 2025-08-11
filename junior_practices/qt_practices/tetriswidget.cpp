/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 09/08/2025, 19:26
 * Description:
 *
 */

#include "tetriswidget.h"

#include <QApplication>
#include <QAudioOutput>
#include <QKeyEvent>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QPainter>
#include <Qurl>
#include <cstdlib>
#include <random>
#include <iostream>
#include <fstream>
#include <string>

TetrisWidget::Point TetrisWidget::coordsTable[7][4] = {
    {{0, 0}, {-1, 0}, {0, -1}, {1, -1}}, // ZShape
    {{0, 0}, {1, 0}, {0, -1}, {-1, -1}}, // SShape
    {{0, 0}, {-1, 0}, {1, 0}, {2, 0}},   // LineShape
    {{0, 0}, {-1, 0}, {0, -1}, {1, 0}},  // TShape
    {{0, 0}, {1, 0}, {0, -1}, {1, -1}},  // SquareShape
    {{0, 0}, {-1, 0}, {-1, -1}, {1, 0}}, // LShape
    {{0, 0}, {1, 0}, {-1, 0}, {1, -1}}   // MirroredLShape
};

TetrisWidget::TetrisWidget(QWidget *parent)
    : QWidget(parent), gen(std::random_device{}()), dist(1, 7), currentPiece(NoShape) {
  // setFixedSize(BoardWidth * blockSize, BoardHeight * blockSize); // 例如 10*30=300
  // 宽度，22*30=660 高度
  setFixedSize(BoardWidth * blockSize + nextBlockAreaWidth, BoardHeight * blockSize);

  clearBoard();
  newPiece();

  highest_score = read_file();

  // 生成随机方块
  int random_num = dist(gen);
  nextPiece = static_cast<Tetrominoes>(random_num);

  connect(&timer, &QTimer::timeout, this, &TetrisWidget::onTimer);
  connect(&blinkTimer, &QTimer::timeout, this, &TetrisWidget::onBlinkTimeout);
  timer.start(500);

  player = new QMediaPlayer(this);
  audioOutput = new QAudioOutput(this);
  player->setSource(
      QUrl::fromLocalFile("D:/chenzhen/Python-Practices/junior_practices/music/background.mp3"));
  player->setAudioOutput(audioOutput);
  audioOutput->setVolume(0.5); // 0.0 ~ 1.0
  player->setLoops(QMediaPlayer::Infinite);
  player->play();
}

void TetrisWidget::clearBoard() {
  for (auto &i : board)
    i = NoShape;
}

bool TetrisWidget::tryMove(const int newX, int newY, int newRotation) {
  Point testCoords[4];
  pieceShapeToCoords(currentPiece, newRotation, testCoords);

  for (int i = 0; i < 4; ++i) {
    int x = newX + testCoords[i].x;
    int y = newY + testCoords[i].y;
    if (x < 0 || x >= BoardWidth || y < 0 || y >= BoardHeight)
      return false;
    if (board[y * BoardWidth + x] != NoShape)
      return false;
  }

  currentX = newX;
  currentY = newY;
  currentRotation = newRotation;
  for (int i = 0; i < 4; ++i)
    coords[i] = testCoords[i];
  return true;
}

void TetrisWidget::newPiece() {
  currentPiece = static_cast<Tetrominoes>((std::rand() % 7) + 1);
  currentRotation = 0;

  pieceShapeToCoords(currentPiece, currentRotation, coords);
  // 找到方块的最高 Y 坐标
  int maxY = coords[0].y;
  for (int i = 1; i < 4; ++i)
    if (coords[i].y > maxY)
      maxY = coords[i].y;

  // X 居中
  currentX = BoardWidth / 2;

  // Y 从棋盘顶部生成（保证整个方块都在棋盘内）
  currentY = BoardHeight - 1;
}

void TetrisWidget::pieceShapeToCoords(const Tetrominoes shape, const int rotation,
                                      Point outCoords[4]) {
  for (int i = 0; i < 4; ++i) {
    Point p = coordsTable[shape - 1][i];
    // 旋转 90 度每次
    for (int r = 0; r < rotation; ++r) {
      const int x = p.x;
      p.x = p.y;
      p.y = -x;
    }
    outCoords[i] = p;
  }
}

void TetrisWidget::dropDown() {
  int dropY = currentY;
  while (tryMove(currentX, dropY - 1, currentRotation)) {
    --dropY;
  }
  pieceDropped();
}

void TetrisWidget::pieceDropped() {
  for (auto &[x, y] : coords) {
    x = currentX + x;
    y = currentY + y;
    board[y * BoardWidth + x] = currentPiece; // enum 数组
  }
  removeFullLines();

  currentPiece = nextPiece;
  currentX = BoardWidth / 2;
  currentY = BoardHeight - 1;
  currentRotation = 0;
  int random_num = dist(gen);
  nextPiece = static_cast<Tetrominoes>(random_num);

  if (!tryMove(currentX, currentY, currentRotation)) {
    QMessageBox::information(this, "Game Over", "Game Over, 游戏结束！");
    timer.stop();
    // 游戏结束处理
    QApplication::quit();
  }

  update(); // 触发重绘
}

void TetrisWidget::removeFullLines() {

  for (int y = BoardHeight - 1; y >= 0; --y) {
    bool lineIsFull = true;
    for (int x = 0; x < BoardWidth; ++x) {
      if (board[y * BoardWidth + x] == NoShape) {
        lineIsFull = false;
        break;
      }
    }
    if (lineIsFull) {
      linesToRemove.append(y);
    }
  }

  if (!linesToRemove.isEmpty()) {
    const size_t line_num = linesToRemove.size();
    linesRemoved += line_num;      // 累加消除的行数
    switch (line_num) {
    case 1:
      score += line_num * 100;
      break;
    case 2:
      score += line_num * 150;
      break;
    case 3:
      score += line_num * 200;
      break;
    case 4:
      score += line_num * 250;
      break;
    case 5:
      score += line_num * 300;
      break;
    default:
      score += line_num * 300;
    }
    blinkCount = 0;
    blinkOn = false;

    // if (!blinkTimer.isActive()) {
    //   connect(&blinkTimer, &QTimer::timeout, this, &TetrisWidget::onBlinkTimeout);
    // }
    blinkTimer.start(150); // 每150ms切换一次闪烁状态
  } else {
    update();
  }
}

void TetrisWidget::onBlinkTimeout() {
  blinkOn = !blinkOn;
  ++blinkCount;

  // 刷新界面，让paintEvent里根据blinkOn状态绘制闪烁效果
  update();

  if (blinkCount >= 3) { // 闪烁3次后消除
    blinkTimer.stop();
    actuallyRemoveLines();
  }
}

void TetrisWidget::actuallyRemoveLines() {
  for (int line : linesToRemove) {
    for (int row = line; row < BoardHeight - 1; ++row) {
      for (int col = 0; col < BoardWidth; ++col) {
        board[row * BoardWidth + col] = board[(row + 1) * BoardWidth + col];
      }
    }
    for (int col = 0; col < BoardWidth; ++col) {
      board[(BoardHeight - 1) * BoardWidth + col] = NoShape;
    }
  }

  linesToRemove.clear();

  clearSound = new QSoundEffect(this);
  clearSound->setSource(
      QUrl::fromLocalFile("D:/chenzhen/Python-Practices/junior_practices/music/clear2.wav"));
  clearSound->setVolume(0.5);
  clearSound->play();

  update();
}

void TetrisWidget::onTimer() {
  if (!tryMove(currentX, currentY - 1, currentRotation)) {
    pieceDropped();
  }
  update();
}

int TetrisWidget::read_file() {
    std::ifstream inFile("high_score.txt"); // 打开分数文件
    if (!inFile) {
      std::cerr << "无法打开 score.txt\n";
      return 1;
    }
    std::string line;
    getline(inFile, line);
    return std::stoi(line);
}

void TetrisWidget::paintEvent(QPaintEvent * /* event */) {
  QPainter painter(this);

  // 先填充深色背景
  painter.fillRect(rect(), QColor(30, 30, 30)); // RGB 深灰色

  // 绘制网格线
  painter.setPen(QPen(QColor(80, 80, 80), 1)); // 浅灰色线条
  for (int x = 0; x <= BoardWidth; ++x) {
    int xpos = x * blockSize;
    painter.drawLine(xpos, 0, xpos, BoardHeight * blockSize);
  }
  for (int y = 0; y <= BoardHeight; ++y) {
    int ypos = y * blockSize;
    painter.drawLine(0, ypos, BoardWidth * blockSize, ypos);
  }

  // 画已经存在的方块
  for (int y = 0; y < BoardHeight; ++y) {
    if (linesToRemove.contains(y) && !blinkOn) {
      // 闪烁状态隐藏时，跳过绘制这行方块，实现闪烁效果
      continue;
    }
    for (int x = 0; x < BoardWidth; ++x) {
      Tetrominoes shape = board[y * BoardWidth + x];
      if (shape != NoShape) {
        drawSquare(painter, x * blockSize, (BoardHeight - y - 1) * blockSize, shape);
      }
    }
  }
  // 正在下降的方块
  if (currentPiece != NoShape) {
    for (int i = 0; i < 4; ++i) {
      int x = currentX + coords[i].x;
      int y = currentY + coords[i].y;
      drawSquare(painter, x * blockSize, (BoardHeight - y - 1) * blockSize, currentPiece);
    }
  }

  // 画“下一个方块”提示框背景
  painter.setPen(Qt::white);
  painter.drawRect(nextBlockAreaX - 10, nextBlockAreaY - 10, 5 * nextBlockSize, 5 * nextBlockSize);
  // 设置字体
  QFont font = painter.font();
  font.setBold(true);
  font.setPointSize(14);
  painter.setFont(font);
  painter.setPen(Qt::white);
  // 计算文字尺寸
  QString text = "Next";
  QFontMetrics fm(font);
  int textWidth = fm.horizontalAdvance(text);
  int textHeight = fm.height();
  // 计算文字位置：提示框宽度 = 5 * nextBlockSize
  int rectX = nextBlockAreaX - 10;
  int rectY = nextBlockAreaY - 10;
  int rectWidth = 5 * nextBlockSize;
  int textX = rectX + (rectWidth - textWidth) / 2; // 水平居中
  int textY = rectY + textHeight + 5;              // 顶部留点边距，靠上

  // 画文字
  painter.drawText(textX, textY, text);

  if (nextPiece != NoShape) {
    Point previewCoords[4];
    pieceShapeToCoords(nextPiece, 0, previewCoords);  // 用旋转后的坐标

    int boxX = nextBlockAreaX - 20;
    int boxY = nextBlockAreaY - 20;

    // 计算预览坐标的边界
    int minX = previewCoords[0].x;
    int maxX = previewCoords[0].x;
    int minY = previewCoords[0].y;
    int maxY = previewCoords[0].y;

    for (int i = 1; i < 4; ++i) {
      if (previewCoords[i].x < minX) minX = previewCoords[i].x;
      if (previewCoords[i].x > maxX) maxX = previewCoords[i].x;
      if (previewCoords[i].y < minY) minY = previewCoords[i].y;
      if (previewCoords[i].y > maxY) maxY = previewCoords[i].y;
    }

    int width = maxX - minX + 1;
    int height = maxY - minY + 1;

    // 偏移调整，居中显示
    int offsetX = (6 - width) / 2 * nextBlockSize - minX * nextBlockSize;
    int offsetY = (6 - height) / 2 * nextBlockSize + maxY * nextBlockSize;  // 注意这里Y轴反转用maxY

    for (int i = 0; i < 4; ++i) {
      int x = boxX + previewCoords[i].x * nextBlockSize; // + offsetX;
      // 预览Y轴翻转，防止镜像
      int y = boxY + previewCoords[i].y * nextBlockSize; // + offsetY;
      drawSquare(painter, x, y, nextPiece);
    }
  }


  // 分数
  painter.setPen(Qt::white);
  font.setPointSize(12);
  painter.setFont(font);
  painter.drawText(nextBlockAreaX, 300, QString("Score: %1").arg(score));
  painter.drawText(nextBlockAreaX, 330, QString("Lines: %1").arg(linesRemoved));
  if (score > highest_score) {
    highest_score = score;
    std::ofstream outFile("high_score.txt");
    outFile << highest_score;
    outFile.close();
  }
  painter.drawText(nextBlockAreaX, 380, QString("Highest: %1").arg(highest_score));
}

void TetrisWidget::drawSquare(QPainter &painter, int x, int y, Tetrominoes shape) {
  static const QColor colors[8] = {
      QColor(Qt::black).lighter(150),  QColor(Qt::red).lighter(150),
      QColor(Qt::green).lighter(150),  QColor(Qt::blue).lighter(150),
      QColor(Qt::cyan).lighter(150),   QColor(Qt::magenta).lighter(150),
      QColor(Qt::yellow).lighter(150), QColor(Qt::gray).lighter(150)};

  QColor color = colors[static_cast<int>(shape)];
  painter.fillRect(x + 1, y + 1, blockSize - 2, blockSize - 2, color);
  painter.setPen(color.lighter());
  painter.drawLine(x, y + blockSize - 1, x, y);
  painter.drawLine(x, y, x + blockSize - 1, y);
  painter.setPen(color.darker());
  painter.drawLine(x + 1, y + blockSize - 1, x + blockSize - 1, y + blockSize - 1);
  painter.drawLine(x + blockSize - 1, y + blockSize - 1, x + blockSize - 1, y + 1);
}

void TetrisWidget::keyPressEvent(QKeyEvent *event) {
  switch (event->key()) {
  case Qt::Key_Left:
    tryMove(currentX - 1, currentY, currentRotation);
    break;
  case Qt::Key_Right:
    tryMove(currentX + 1, currentY, currentRotation);
    break;
  case Qt::Key_Up:
    tryMove(currentX, currentY, (currentRotation + 1) % 4);
    rotateSound = new QSoundEffect(this);
    rotateSound->setSource(
        QUrl::fromLocalFile("D:/chenzhen/Python-Practices/junior_practices/music/rotate.wav"));
    rotateSound->setVolume(0.5);
    rotateSound->play();
    break;
  case Qt::Key_Down:
    tryMove(currentX, currentY - 1, currentRotation);
    break;
  case Qt::Key_Space:
    dropDown();
    break;
  default:
    QWidget::keyPressEvent(event);
  }
  update();
}
