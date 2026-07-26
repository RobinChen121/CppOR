/**
 * Author: Zhen Chen
 * Email: chen.zhen5526@gmail.com
 * Created on: 24/06/2026, 22:59
 * Description:
 *
 */

#ifndef CHEN_SOLVER_JS_READ_FILE_H
#define CHEN_SOLVER_JS_READ_FILE_H

#include "model.h"
#include <sstream>

Model readLP(const std::string &path);
Model readMPS(const std::string &path);
Model read(const std::string &path);

#endif // CHEN_SOLVER_JS_READ_FILE_H
