#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/zhenchen/CLionProjects/CppOR/stochastic_inventory/newsvendor/build
  make -f /Users/zhenchen/CLionProjects/CppOR/stochastic_inventory/newsvendor/build/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/zhenchen/CLionProjects/CppOR/stochastic_inventory/newsvendor/build
  make -f /Users/zhenchen/CLionProjects/CppOR/stochastic_inventory/newsvendor/build/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/zhenchen/CLionProjects/CppOR/stochastic_inventory/newsvendor/build
  make -f /Users/zhenchen/CLionProjects/CppOR/stochastic_inventory/newsvendor/build/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/zhenchen/CLionProjects/CppOR/stochastic_inventory/newsvendor/build
  make -f /Users/zhenchen/CLionProjects/CppOR/stochastic_inventory/newsvendor/build/CMakeScripts/ReRunCMake.make
fi

