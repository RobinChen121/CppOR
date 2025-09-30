cmake_minimum_required(VERSION 3.30)
project(tsp) # 设置运行程序的名字

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
        tsp.h
        #        DP.cpp
        branch_and_bound2.cpp
        #        branch_and_bound1.cpp
)