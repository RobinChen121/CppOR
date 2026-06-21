cmake_minimum_required(VERSION 3.30)
project(highs) # 设置运行程序的名字

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
        test_mps.cpp
)

# highs setting
# Provide a hint to where HiGHS is located on your machine
# (Replace this path with the actual path to your HiGHS build or install directory)
set(highs_DIR "D:/chenzhen/HiGHS/build" CACHE PATH "Path to HiGHS config" FORCE)
find_package(highs REQUIRED)
target_link_libraries(${PROJECT_NAME} highs::highs)