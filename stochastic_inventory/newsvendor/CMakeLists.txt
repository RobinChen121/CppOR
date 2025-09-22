cmake_minimum_required(VERSION 3.30)
project(newsvendor) # 设置运行程序的名字

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(CMAKE_BUILD_TYPE Debug)
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

        #        heuristic1997.cpp
        newsvendor.cpp

        ../../utils/draw_graph.cpp
        ../../utils/Kconvexity.cpp
)


#-----------------------------------
# Boost
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})


#-----------------------------------
# matplotlib setting
if (MSVC)
    add_definitions(-U_DEBUG)  # 取消 _DEBUG 宏,这样jiuneg
endif ()
if (APPLE)
    # python head file
    include_directories(/opt/anaconda3/include/python3.12)
    # numpy head file
    include_directories(/opt/anaconda3/lib/python3.12/site-packages/numpy/core/include)
    target_link_libraries(${PROJECT_NAME}
            /opt/anaconda3/lib/libpython3.12.dylib
            dl
    )
elseif (MSVC)
    include_directories(C:/ProgramData/anaconda3/include/)
    include_directories(C:/ProgramData/anaconda3/Lib/site-packages/numpy/core/include/)
    target_link_libraries(${PROJECT_NAME}
            optimized C:/ProgramData/anaconda3/libs/python312.lib # Release 模式用 python312.lib
            debug C:/ProgramData/anaconda3/libs/python312.lib # Debug 模式也强制用 python312.lib
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