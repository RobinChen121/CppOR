/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 20/09/2025, 15:16
 * Description:
 * 直接调用 python api 跟用 matplotlibcpp 效果差不多，主要用 PyRun_SimpleString，
 * 将 python 代码用字符串形式传递给它
 */
#include <Python.h>

int main() {
  // 1️⃣ 初始化 Python 解释器
  Py_Initialize();

  // 2️⃣ 导入 Python 模块
  // TkAgg 通常更稳定，不容易阻塞
  PyRun_SimpleString("import matplotlib; matplotlib.use('TkAgg')");
  PyRun_SimpleString("import matplotlib.pyplot as plt");
  PyRun_SimpleString("import numpy as np");

  // 3️⃣ 定义 x 和 y 数据
  PyRun_SimpleString("x = np.linspace(0, 2*np.pi, 100)\n"
                     "y = np.sin(x)");

  // 4️⃣ 绘制图形
  PyRun_SimpleString("plt.plot(x, y)\n"
                     "plt.title('y = sin(x)')\n"
                     "plt.xlabel('x')\n"
                     "plt.ylabel('y')\n"
                     "plt.grid(True)\n"
                     "plt.show()");

  // 5️⃣ 结束 Python 解释器
  Py_Finalize();

  return 0;
}
