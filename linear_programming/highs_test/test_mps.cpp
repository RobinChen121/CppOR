/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 21/06/2026, 17:30
 * Description:
 *
 */
#include "Highs.h"

int main() {
  Highs highs;

  highs.readModel("D:/chenzhen/CppOR/linear_programming/highs_test/test_sets/afiro.mps");

  highs.run();
  // highs.writeModel("afiro.lp");
}