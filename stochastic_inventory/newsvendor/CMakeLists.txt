cmake_minimum_required(VERSION 3.30)
project(newsvendor) # 设置运行程序的名字

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else ()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif ()

# 为不同平台配置 ASan 标志
# 启用会影响速度
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (APPLE)
        # Mac 上的 Clang
        add_compile_options(-fsanitize=address -g -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    elseif (MSVC)
        # Windows 上的 MSVC
        # /fsanitize=address 会自动包含调试信息并优化堆栈帧
        add_compile_options(/fsanitize=address)
        # 注意：MSVC 不需要额外的链接标志，编译器会自动处理
    else ()
        # Linux 上的 GCC/Clang (通用处理)
        add_compile_options(-fsanitize=address -g -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif ()
endif ()

# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${PROJECT_NAME}
        ../../utils/pmf.cpp
        #        ../states/state.cpp

        #        newsvendor_c.c
        #        newsvendor_vanilla.c
        #        heuristic1997.cpp
        #                newsvendor_capacity.cpp
        newsvendor_1Dvector.cpp
        #        newsvendor_vanilla.cpp
        #        newsvendor_newhash.cpp
        #        newsvendor.h
        #
        #        ../../utils/draw_graph.cpp
        #        ../../utils/Kconvexity.cpp
        ../../utils/pmf.cpp
        ../../linear_programming/chen_solver/include/chen_solver/core/matrix.h
        ../../linear_programming/chen_solver/include/chen_solver/config.h
        ../../linear_programming/chen_solver/include/chen_solver/core/variable.cpp
        ../../linear_programming/chen_solver/include/chen_solver/core/variable.h
        ../../linear_programming/chen_solver/include/chen_solver/configs.h
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
    # 1. 寻找 Boost 库
    # 如果你需要特定的组件（如 system, filesystem），在后面加上 COMPONENTS
    find_package(Boost REQUIRED)

    # 2. 包含头文件路径
    include_directories(${Boost_INCLUDE_DIRS})

    # 3. 链接库
    # Boost::boost 是一个虚拟目标，它会自动处理头文件搜索路径
    # 避免 boost 升级路径错误
    target_link_libraries(${PROJECT_NAME}
            Boost::boost
    )
endif ()


#-----------------------------------
# matplotlib setting
if (MSVC)
    add_definitions(-U_DEBUG)
endif ()
if (APPLE)
    # python head file
    include_directories(/opt/anaconda3/include/python3.13)
    # numpy head file
    include_directories(/opt/anaconda3/lib/python3.13/site-packages/numpy/_core/include)
    target_link_libraries(${PROJECT_NAME}
            /opt/anaconda3/lib/libpython3.13.dylib
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

#-----------------------------------
# OpenMP needs to be downloaded to the mac, parallel computing api
if (APPLE)
    set(OpenMP_INCLUDE_DIR "/opt/homebrew/Cellar/libomp/21.1.8/include")
    set(OpenMP_LIBRARY "/opt/homebrew/Cellar/libomp/21.1.8/lib") # /libomp.dylib
    # 手动设置 OpenMP 相关的编译和链接选项
    set(OpenMP_CXX_LIBS "-L${OpenMP_LIBRARY} -lomp")
    set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I${OpenMP_INCLUDE_DIR}")
    # 应用 OpenMP 选项
    # 这两行命令使得运行当前并行计算 cpp 等同于在命令行中的设置
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}") # CMAKE_CXX_FLAGS 变量用于存储 C++ 编译器的全局编译选项（flags）
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_CXX_LIBS}") # CMAKE_EXE_LINKER_FLAGS 变量用于存储 链接时的额外选项
    # 添加 OpenMP 头文件路径和库路径
    include_directories(${OpenMP_INCLUDE_DIR})
else ()
    find_package(OpenMP REQUIRED)
    if (MSVC)
        target_compile_options(junior_practice PRIVATE /openmp)
    else ()
        target_compile_options(${PROJECT_NAME} PRIVATE -fopenmp)
        target_link_options(${PROJECT_NAME} PRIVATE -fopenmp)
    endif ()
    if (OpenMP_CXX_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
    endif ()
endif ()