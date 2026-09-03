/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 07/08/2025, 20:51
 * Description:
 *
 */

#ifndef UTIL_BINOMIAL_H
#define UTIL_BINOMIAL_H

double binomialPDF(int n, double p, int k);
double binomialCDF(int n, double p, int k);
double lossFunctionExpect(int y, int min_worker, double turnover_rate);
double Fy_y_minus_w(int y, int min_worker, double turnover_rate);
#endif // UTIL_BINOMIAL_H
