cmake_minimum_required(VERSION 3.30)
Project(newsvendor) # 设置运行程序的名字

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_BUILD_TYPE STREQUAL Release)
    if (MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "/O3")
    else ()
        set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    endif ()
else ()
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
endif ()

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -gdwarf-3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

#set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer -pthread")
# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${PROJECT_NAME}
        ../../utils/pmf.cpp
        ../states/State.cpp

        newsvendor.cpp
        #        newsvendor_parallel.cpp
        #        heuristic1997.cpp
        #        cash/CashState.cpp
)

#-----------------------------------
# Boost
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
