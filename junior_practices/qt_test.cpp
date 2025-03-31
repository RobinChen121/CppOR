/*
 * Created by Zhen Chen on 2025/3/25.
 * Email: chen.zhen5526@gmail.com
 * Description:
 *
 *
 */
#include <QApplication>
#include <QButtonGroup>
#include <QDebug>
#include <QFormLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QRadioButton>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QWidget window;
  window.setWindowTitle("Dr Zhen Chen's solver");
  window.resize(800, 600);

  // 创建一个 QLabel 控件并设置文本
  auto *label_decision_num =
      new QLabel("Please input your number of decision variables:", &window);
  label_decision_num->adjustSize(); // 让 QLabel 自动调整大小，以适应文本内容

  auto *label_sense = new QLabel("Your objective sense:", &window);
  label_sense->setContentsMargins(20, 0, 0, 0);
  label_sense->adjustSize();

  // 创建输入框
  auto *lineEdit = new QLineEdit(&window);
  // lineEdit->setPlaceholderText("Press enter after inputting...");
  // 设置输入框仅接受数字
  lineEdit->setValidator(
      new QIntValidator(0, 100, lineEdit)); // 只允许输入 0-100 之间的整数
  lineEdit->setMaximumWidth(50);

  // 监听输入完成事件
  int var_num = 0;
  var_num = lineEdit->text().toInt();
  // QObject::connect(
  //     lineEdit, &QLineEdit::editingFinished, [lineEdit, &var_num]() {
  //       qDebug() << "Decision variable number：" << lineEdit->text();
  //       var_num = lineEdit->text().toInt();
  //       lineEdit->setReadOnly(true); // 设置为只读，不允许再输入
  //     });

  // 创建单选按钮
  auto *radio1 = new QRadioButton("Min");
  auto *radio2 = new QRadioButton("Max");
  // 创建按钮组，确保按钮互斥
  auto *buttonGroup = new QButtonGroup(&window);
  buttonGroup->addButton(radio1);
  buttonGroup->addButton(radio2);
  // 默认选中第一个
  radio1->setChecked(true);

  // 创建水平布局（让 Qlabel 和 QLineEdit 紧挨着）
  auto *hLayout = new QHBoxLayout();
  hLayout->addWidget(label_decision_num);
  hLayout->addWidget(lineEdit);
  hLayout->addWidget(label_sense);
  hLayout->addWidget(radio1);
  hLayout->addWidget(radio2);
  hLayout->addStretch(); // 在右侧添加“弹性”空间，让前面的内容靠左

  auto *vLayout = new QVBoxLayout(&window);
  vLayout->addLayout(hLayout); // 添加 hLayout

  auto *label_obj =
      new QLabel("Please input your objective coefficients:", &window);
  vLayout->addWidget(label_obj); // 新的一行添加控件
  for (auto i = 0; i < var_num; i++) {
    // 创建 QLabel 控件
    auto *label = new QLabel(&window);
    // 设置为 LaTeX 公式格式的文本
    label->setText("x<sub>" + QString::number(i));
    // 将 QLabel 添加到布局中
    vLayout->addWidget(label);
  }

  // 这段代码会让 hLayout 距离顶部 50px
  vLayout->addSpacerItem(
      new QSpacerItem(0, 50, QSizePolicy::Minimum, QSizePolicy::Expanding));

  window.setLayout(vLayout);
  window.show();

  return app.exec();
}
