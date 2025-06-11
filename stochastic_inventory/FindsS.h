/*
 * Created by Zhen Chen on 2025/6/11.
 * Email: chen.zhen5526@gmail.com
 * Description: 
 *
 *
 */

#ifndef FINDSS_H
#define FINDSS_H

#include<vector>

class FindsS {
  int maxQ;
  int T;

  public:
    FindsS(int maxQ, int T): maxQ(maxQ), T(T) {};

    std::vector<int> sIndex(std::vector<std::vector<const double>> opt_table) const;
};



#endif //FINDSS_H
