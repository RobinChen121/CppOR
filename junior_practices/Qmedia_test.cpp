/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 09/08/2025, 20:58
 * Description:
 *
 */
#include <QApplication>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QUrl>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  auto *player = new QMediaPlayer;
  auto *audioOutput = new QAudioOutput;
  player->setSource(
      QUrl::fromLocalFile("D:/chenzhen/Python-Practices/junior_practices/music/background.mp3"));
  player->setAudioOutput(audioOutput);
  audioOutput->setVolume(0.5); // 0.0 ~ 1.0
  player->setLoops(QMediaPlayer::Infinite);
  player->play();
  return QApplication::exec();
}
