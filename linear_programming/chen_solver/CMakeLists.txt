cmake_minimum_required(VERSION 3.30)
project(chen_solver) # 设置运行程序的名字

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(CMAKE_BUILD_TYPE Release)
if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else ()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif ()

# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${PROJECT_NAME}
        simplex.cpp

)

# mac:
# 1. 激活环境，命令行输入：source /Users/zhenchen/emsdk/emsdk_env.sh
# 2. 然后运行下面命令
# emcc simplex.cpp -o simplex.js -std=c++20 -s MODULARIZE=1 -s EXPORT_NAME="simplexModule" --bind -O3

# windows:
# 编译cpp为js:
# 1. 激活环境
# D:\chenzhen\emsdk-main\emsdk_env.bat
# 2. 然后运行下面命令
# emcc simplex.cpp -o simplex.js -std=c++20 -s MODULARIZE=1 -s EXPORT_NAME="simplexModule" --bind -O3

# 或者按下面步骤生成到那个build 文件夹里
## Emscripten 的编译设置
#set_target_properties(${PROJECT_NAME} PROPERTIES
#        LINK_FLAGS "--bind -s MODULARIZE=1 -s EXPORT_NAME=simplexModule"
#        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/dist"
#)
# 有时需要重新删除那个 build 文件夹
# 1. 激活环境
# D:\chenzhen\emsdk-main\emsdk_env.bat
# 2. 配置
# emcmake cmake -B build -S .
# 3. 编译
# cmake --build build

# 这几个路径可能没啥用
#include_directories(
#        "D:/chenzhen/emsdk-main/upstream/emscripten/system/include"
#        "D:/chenzhen/emsdk-main/upstream/emscripten/system/include/libcxx"
#        "D:/chenzhen/emsdk-main/upstream/emscripten/system/include/libc"
#)

