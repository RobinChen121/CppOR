/*
 * Created by Zhen Chen on 2025/3/26.
 * Email: chen.zhen5526@gmail.com
 * Description: Dynamic label generator based on user input
 */
#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class LabelGenerator : public QWidget {
  Q_OBJECT
public:
  LabelGenerator(QWidget *parent = nullptr) : QWidget(parent) {
    // 初始化界面组件
    inputLineEdit = new QLineEdit(this);
    inputLineEdit->setPlaceholderText("请输入标签数量");
    generateButton = new QPushButton("生成标签", this);
    labelLayout = new QVBoxLayout();

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(inputLineEdit);
    mainLayout->addWidget(generateButton);
    mainLayout->addLayout(labelLayout);
    mainLayout->addStretch();

    // 连接信号和槽，并验证
    bool connected = connect(generateButton, &QPushButton::clicked, this,
                             &LabelGenerator::generateLabels);
    qDebug() << "Signal connected:" << connected;

    setLayout(mainLayout);
    setWindowTitle("动态生成标签");
  }

private slots:
  void generateLabels() {
    clearLabels();
    bool ok;
    int count = inputLineEdit->text().toInt(&ok);
    if (!ok || count <= 0) {
      QMessageBox::warning(this, "错误", "请输入一个正整数！");
      return;
    }
    qDebug() << "Generating" << count << "labels";
    for (int i = 0; i < count; ++i) {
      QLabel *label = new QLabel(QString("标签 %1").arg(i + 1), this);
      labelList.append(label);
      labelLayout->addWidget(label);
    }
  }

private:
  void clearLabels() {
    qDebug() << "Clearing" << labelList.size() << "labels";
    for (QLabel *label : labelList) {
      labelLayout->removeWidget(label);
      delete label;
    }
    labelList.clear();
  }

  QLineEdit *inputLineEdit;
  QPushButton *generateButton;
  QVBoxLayout *labelLayout;
  QList<QLabel *> labelList;
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  LabelGenerator window;
  window.resize(300, 400);
  window.show();
  return app.exec();
}

#include "qt_test2.moc"