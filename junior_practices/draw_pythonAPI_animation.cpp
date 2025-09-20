/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 20/09/2025, 15:27
 * Description:
 *
 */
#include <Python.h>
#include <string>

int main() {
  // 1️⃣ 初始化 Python
  Py_Initialize();

  // 2️⃣ 强制使用 TkAgg 后端，避免 Qt6Agg 卡死
  PyRun_SimpleString("import matplotlib; matplotlib.use('TkAgg')");
  PyRun_SimpleString("import matplotlib.pyplot as plt");
  PyRun_SimpleString("import numpy as np");
  PyRun_SimpleString("import time");

  // 3️⃣ 初始化图形
  // PyRun_SimpleString("plt.ion()"); // 开启交互模式
  PyRun_SimpleString("fig, ax = plt.subplots()");
  PyRun_SimpleString("x = np.linspace(0, 2*np.pi, 200)");
  PyRun_SimpleString("line, = ax.plot(x, np.sin(x))");
  PyRun_SimpleString("ax.set_ylim(-1.5, 1.5)");

  // 4️⃣ 在 C++ 循环里调用 Python 更新数据
  for (int i = 0; i < 200; i++) {
    std::string cmd = "line.set_ydata(np.sin(x + " + std::to_string(i * 0.1) +
                      "))\n"
                      "plt.draw()\n" // plt.draw() 刷新图形
                      "plt.pause(0.05)";
    PyRun_SimpleString(cmd.c_str()); // PyRun_SimpleString 需要一个 C 风格字符串 (const char*)
                                     // 作为输入,因此需要c_str()函数转化
  }

  // 5️⃣ 等待用户关闭窗口
  PyRun_SimpleString("plt.show()");
  // PyRun_SimpleString("plt.ioff()\nplt.show()");

  // 6️⃣ 关闭 Python
  Py_Finalize();
  return 0;
}
