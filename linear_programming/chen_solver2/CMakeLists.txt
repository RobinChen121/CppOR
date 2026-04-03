cmake_minimum_required(VERSION 3.30)
project(chen_solver2) # 设置运行程序的名字

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(CMAKE_BUILD_TYPE Debug)
if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else ()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif ()

# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${PROJECT_NAME}
        simplex.cpp

)

# windows:
# 编译cpp为js:
# 1. 激活环境
# D:\chenzhen\emsdk-main\emsdk_env.bat
# 2. 然后在命令行运行下面命令
# python D:\chenzhen\emsdk-main\upstream\emscripten\emcc.py simplex.cpp -o simplex.js -std=c++20 -s MODULARIZE=1 -s EXPORT_NAME="simplexModule" -s ALLOW_MEMORY_GROWTH=1 --bind -O3
