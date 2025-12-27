cmake_minimum_required(VERSION 3.30)
project(sdp-inventory)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(cash)
add_subdirectory(states)
add_subdirectory(workforce)


#set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer -pthread")
# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${PROJECT_NAME}
        ../utils/pmf.cpp
        states/state.cpp
        newsvendor/newsvendor.cpp
        #        cash/CashState.cpp

)

#-----------------------------------
# Boost
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
