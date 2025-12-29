cmake_minimum_required(VERSION 3.30)
project(sdp-overdraft) # 设置运行程序的名字

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Release) # 运行 Debug 还是 Release

#set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer -pthread")
# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${PROJECT_NAME}
        ../../../utils/pmf.cpp
        ../../../utils/common.cpp

        ../../states/state.cpp
        ../../states/cash_state.cpp
        ../../states/cash_leadtime_state.cpp

        ../../states/cash_leadtime_multi_state.cpp
        #        leadtime_double_product.cpp
        #        leadtime_single_product_testing.cpp
        leadtime_single_product.cpp

        #        leadtime_single_product_testing.cpp

)

#-----------------------------------
# Boost
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})