set(RUN_NAME workforce) # 设置运行程序的名字

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_BUILD_TYPE Debug) # 运行 Debug 还是 Release
#set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer -pthread")
# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${RUN_NAME}
        worker_state.cpp
        workforce_plan.cpp

        #        workforce_testing_animation.cpp
        ../../utils/pmf.cpp
        ../../utils/draw_graph.cpp
        #        binomial_product_test.cpp
        #        Binomial_conditional_expectation_test.cpp
        #        Binomial_stochastic_order_test.cpp
        #        Binomial_probability_decomposition_test.cpp
        #        Binomial_logconcave_test.cpp
        piecewise.cpp
)


## 设置 Gurobi 路径（根据你的安装调整）
if (APPLE)
    set(GUROBI_HOME "/Library/gurobi1201/macos_universal2") # gurobi 的安装地址
    message("test")
else ()
    set(GUROBI_HOME "C:/gurobi1202/win64") # gurobi 的安装地址
endif ()
set(GUROBI_INCLUDE_DIR "${GUROBI_HOME}/include") # 头文件路径
set(GUROBI_LIB_DIR "${GUROBI_HOME}/lib") # 库文件路径

# 将路径添加到搜索地址
include_directories(${GUROBI_LIB_DIR})
include_directories(${GUROBI_INCLUDE_DIR})

# 链接 Gurobi 库，这个静态库与动态库都需要
if (APPLE)
    target_link_libraries(${RUN_NAME}
            PRIVATE
            "${GUROBI_LIB_DIR}/libgurobi_c++.a" # .a 是静态库
            "${GUROBI_LIB_DIR}/libgurobi120.dylib" # .dylib 是动态库
    )
else ()
    target_link_libraries(${RUN_NAME}
            PRIVATE
            "${GUROBI_INCLUDE_DIR}/gurobi_c++.h"
            "${GUROBI_LIB_DIR}/gurobi120.lib" #
    )
endif ()


##-----------------------------------
## fmt for format output
#find_package(fmt REQUIRED)
#target_link_libraries(${RUN_NAME} PRIVATE fmt::fmt)

#-----------------------------------
# Boost
# Boost 提供了很多通用的的库，例如数值计算等
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
target_link_libraries(${RUN_NAME} PRIVATE ${Boost_LIBRARIES})

#-----------------------------------
# matplotlib
# 设置 Python 头文件路径
if (APPLE)
    include_directories(/opt/anaconda3/include/python3.12)
    # numpy 头文件路径
    include_directories(/opt/anaconda3/lib/python3.12/site-packages/numpy/core/include)
    # 链接所需库
    target_link_libraries(${RUN_NAME} PRIVATE
            /opt/anaconda3/lib/libpython3.12.dylib
            dl
    )
else ()
    FetchContent_Declare(
            matplotlib_cpp
            GIT_REPOSITORY https://github.com/lava/matplotlib-cpp.git
            GIT_TAG master
    )
    FetchContent_MakeAvailable(matplotlib_cpp)
    # Include matplotlib-cpp
    target_include_directories(${RUN_NAME} PRIVATE ${matplotlib_cpp_SOURCE_DIR})
    # Link Python and NumPy
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
    target_include_directories(${RUN_NAME} PRIVATE
            ${Python3_INCLUDE_DIRS}
            ${Python3_NumPy_INCLUDE_DIRS}
    )
    target_link_libraries(${RUN_NAME} PRIVATE
            ${Python3_LIBRARIES}
    )
endif ()

#-----------------------------------
# OpenMP is also too large and needs to be downloaded to the computer yourself first
# parallel computing api
if (APPLE)
    set(OpenMP_INCLUDE_DIR "/opt/homebrew/Cellar/libomp/20.1.0/include")
    set(OpenMP_LIBRARY "/opt/homebrew/Cellar/libomp/20.1.0/lib") # /libomp.dylib
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
    target_compile_options(${RUN_NAME} PRIVATE -fopenmp)
    target_link_options(${RUN_NAME} PRIVATE -fopenmp)
    if (OpenMP_CXX_FOUND)
        target_link_libraries(${RUN_NAME} PRIVATE OpenMP::OpenMP_CXX)
    endif ()
endif ()