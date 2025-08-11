/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 09/08/2025, 19:26
 * Description:
 *
 */
#ifndef TETRISWIDGET_H
#define TETRISWIDGET_H

#include <QMediaPlayer>
#include <QSoundEffect>
#include <QTimer>
#include <QWidget>
#include <random>

// 共有继承，默认是 private 继承
class TetrisWidget final : public QWidget {
  Q_OBJECT // 支持 QObject::connect() 动态连接信号和槽

public:
  explicit TetrisWidget(QWidget *parent = nullptr);

protected:
  void paintEvent(QPaintEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

private slots:
  void onTimer();

private:
  // 在类的成员变量里，定义随机引擎和分布器（推荐只初始化一次）
  std::mt19937 gen;
  std::uniform_int_distribution<int> dist;

  int highest_score;
  bool update_highest_score = false;

  QTimer blinkTimer;
  QVector<int> linesToRemove;   // 要消除的行
  int blinkCount = 0;           // 记录闪烁次数
  bool blinkOn = false;         // 当前闪烁状态（显示或隐藏）

  QMediaPlayer *player;
  QAudioOutput *audioOutput;
  QSoundEffect *rotateSound;
  QSoundEffect *clearSound;
  enum Tetrominoes {
    NoShape,
    ZShape,
    SShape,
    LineShape,
    TShape,
    SquareShape,
    LShape,
    MirroredLShape
  };

  Tetrominoes nextPiece; // 下一个方块
  void drawNextPiece(QPainter &painter); // 画下个方块的小函数

  struct Point {
    int x;
    int y;
  };

  int score = 0;
  int linesRemoved = 0;   // 累计消除行数
  void clearBoard();
  bool canMove(Tetrominoes shape, int rotation, int newX, int newY) const;
  void newPiece();
  static void pieceShapeToCoords(Tetrominoes shape, int rotation, Point outCoords[4]);
  bool tryMove(int newX, int newY, int newRotation);
  void dropDown();
  void pieceDropped();
  void removeFullLines();
  void onBlinkTimeout();
  void actuallyRemoveLines();
  void drawSquare(QPainter &painter, int x, int y, Tetrominoes shape);

  static constexpr int BoardWidth = 10;
  static constexpr int BoardHeight = 20;
  static constexpr int blockSize = 30;
  static constexpr int nextBlockAreaWidth = 180;
  static constexpr int nextBlockAreaX = BoardWidth * blockSize + 20;  // 在主棋盘右边20像素处开始
  static constexpr int nextBlockAreaY = 30;  // 你可以自己调整
  static constexpr int nextBlockSize = 30;  // 下个方块每个小格大小，比主方块小一点

  static Point coordsTable[7][4];

  Tetrominoes board[BoardWidth * BoardHeight]{};
  Tetrominoes currentPiece;
  Point coords[4]{};
  int currentX{};
  int currentY{};
  int currentRotation{};

  static int read_file();
  void write_file();

  QTimer timer;
};

#endif // TETRISWIDGET_H
