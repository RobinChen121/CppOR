/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 09/08/2025, 19:36
 * Description: 
 * 
 */
#include <QApplication>
#include "tetriswidget.h"

int main(int argc, char *argv[]) {
  // QApplication 的构造函数需要接收 argc 和 argv，以便它能处理一些命令行参数（比如主题、样式、调试参数等）
  // Must construct a QApplication before a QWidget class
  // Qt GUI 程序中只能有一个 QApplication 实例
  // QApplication 管理的是整个应用，而不是某个组件
  QApplication a(argc, argv);
  TetrisWidget w;
  w.show(); // QWidget 自带的函数 show(),
  return QApplication::exec();
}
