cmake_minimum_required(VERSION 3.30)
project(newsvendor) # 设置运行程序的名字

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else ()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif ()


# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${PROJECT_NAME}
        #        ../../utils/pmf.cpp
        #        ../states/state.cpp

        #        newsvendor_c.c
        #        newsvendor_vanilla.c
        #        heuristic1997.cpp
        #        newsvendor_capacity.cpp
        newsvendor_vanilla.cpp
        #        newsvendor.h
        #
        #        ../../utils/draw_graph.cpp
        #        ../../utils/Kconvexity.cpp
        #        ../../tsp/tsp.h
        #        ../../tsp/DP.cpp
)


#-----------------------------------
# Boost
if (MSVC)
    target_include_directories(${PROJECT_NAME}
            PRIVATE
            D:/local/boost_1_90_0
    )
else ()
    find_package(Boost REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
endif ()


#-----------------------------------
# matplotlib setting
if (MSVC)
    add_definitions(-U_DEBUG)
endif ()
if (APPLE)
    # python head file
    include_directories(/opt/anaconda3/envs/py312/include/python3.12)
    # numpy head file
    include_directories(/opt/anaconda3/envs/py312/lib/python3.12/site-packages/numpy/_core/include)
    target_link_libraries(${PROJECT_NAME}
            /opt/anaconda3/envs/py312/lib/libpython3.12.dylib
            dl
    )
elseif (MSVC)
    find_package(Python3 REQUIRED COMPONENTS Development NumPy)

    target_include_directories(${PROJECT_NAME} PRIVATE
            ${Python3_INCLUDE_DIRS}
            ${Python3_NumPy_INCLUDE_DIRS}
    )

    target_link_libraries(${PROJECT_NAME}
            ${Python3_LIBRARIES}
    )
else ()
    FetchContent_Declare(
            matplotlib_cpp
            GIT_REPOSITORY https://github.com/lava/matplotlib-cpp.git
            GIT_TAG master
    )
    FetchContent_MakeAvailable(matplotlib_cpp)
    # Include matplotlib-cpp
    target_include_directories(${PROJECT_NAME} ${matplotlib_cpp_SOURCE_DIR})
    # Link Python and NumPy
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
    target_include_directories(${PROJECT_NAME}
            ${Python3_INCLUDE_DIRS}
            ${Python3_NumPy_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME}
            ${Python3_LIBRARIES}
    )
endif ()