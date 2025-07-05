set(RUN_NAME junior_practice) # the name of configuration target
if (POLICY CMP0153)
    cmake_policy(SET CMP0153 OLD)
endif ()


add_executable(${RUN_NAME}
        #        pair_test.cpp
        #       spdlog_test.cpp
        #        eigen3_test.cpp
        draw_animation.cpp
        #        qt_test.cpp
        #        openMP_test.cpp
)

include(FetchContent) # it can fetch content automatically

# Qt is too large, can't use FetchContent for it and should be download to
# the computer first yourself
# 设置 Qt 安装路径
if (APPLE)
    message("mac")
else ()
    set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/mingw_64")
endif ()
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
if (Qt6Core_FOUND AND Qt6Widgets_FOUND)
    message(STATUS "Qt6 Core and Widgets found.")
else ()
    message(FATAL_ERROR "Qt6 components not found.")
endif ()
# link Qt
target_link_libraries(${RUN_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

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


#-----------------------------------
# Eigen3
# 高性能、跨平台的 C++ 线性代数库
# 下载 Eigen3（官方 GitHub）
#FetchContent_Declare(
#        eigen
#        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
#        GIT_TAG        3.4.0  # 或最新版本
#)
#FetchContent_MakeAvailable(eigen)
## 不需要 link，只需要 include
#target_include_directories(${RUN_NAME} PRIVATE ${eigen_SOURCE_DIR})

# boost
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
target_link_libraries(${RUN_NAME} PRIVATE ${Boost_LIBRARIES})

#-----------------------------------
# spdlog
# 日志库,这个不适合 fetchcontent
#FetchContent_Declare(
#        spdlog
#        GIT_REPOSITORY https://github.com/gabime/spdlog.git
#        GIT_TAG        v1.13.0  # 指定版本号
#)
#FetchContent_MakeAvailable(spdlog)
#target_link_libraries(${RUN_NAME} PRIVATE spdlog::spdlog)

#-----------------------------------
# 下面为调用 matplotlibcpp 的设置
if (APPLE)
    # 设置 Python 头文件路径
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


if (APPLE)
    find_package(Eigen3 REQUIRED)
    target_link_libraries(${RUN_NAME} PRIVATE Eigen3::Eigen)
else ()
    set(EIGEN3_INCLUDE_DIR "D:/c++Library/eigen-3.4.0")
    target_include_directories(${RUN_NAME} PRIVATE ${EIGEN3_INCLUDE_DIR})
endif ()

if (APPLE)
    find_package(spdlog)
    target_link_libraries(${RUN_NAME} PRIVATE spdlog::spdlog)
else ()
    # 添加头文件路径
    target_include_directories(${RUN_NAME} PRIVATE "D:/c++Library/spdlog/include")
endif ()

