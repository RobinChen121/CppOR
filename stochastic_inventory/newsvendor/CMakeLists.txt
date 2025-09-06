cmake_minimum_required(VERSION 3.30)
project(newsvendor) # 设置运行程序的名字

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(CMAKE_BUILD_TYPE Release)
if (CMAKE_BUILD_TYPE STREQUAL Release)
    if (MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "/Ox")
    else ()
        set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    endif ()
else ()
    if (MSVC)
        set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi")
    else ()
        set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
    endif ()
endif ()



# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${PROJECT_NAME}
        ../../utils/pmf.cpp
        ../states/State.cpp

        heuristic1997.cpp
#                newsvendor.cpp
)


#-----------------------------------
# Boost
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
