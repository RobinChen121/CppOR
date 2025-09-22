cmake_minimum_required(VERSION 3.30)
project(practice)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RUN_NAME junior_practice) # the name of configuration target

#set(CMAKE_BUILD_TYPE Release)
if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else ()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif ()

#add_subdirectory(qt_practices)
#add_subdirectory(SDL2_practices)

add_executable(${RUN_NAME}
        #        Qmedia_test.cpp
        #        SFML_practices/main.cpp

        #        ../utils/matplotlibcpp.h
        draw_python.cpp
        #        draw_python_API.cpp
        #        draw_pythonAPI_animation.cpp
        #        draw_python2.cpp
        #        draw_animation.cpp

        #        boost_test.cpp
        #        game_tetris.cpp
        #        pair_test.cpp
        #               spdlog_test.cpp
        #        eigen3_test.cpp

        #        draw_QPainter_bar.cpp
        #        draw_QPainter_line.cpp
        #                variadic-test.cpp
        #                qt_test.cpp

        #                openMP_test2.cpp
)

# boost setting
if (MSVC)
    set(BOOST_ROOT "C:/local/boost_1_88_0")
    set(BOOST_INCLUDEDIR "${BOOST_ROOT}")
    set(BOOST_LIBRARYDIR "${BOOST_ROOT}/stage/lib")
    include_directories(${BOOST_INCLUDEDIR})
    link_directories(${BOOST_LIBRARYDIR})
    set(BOOST_LIBS
            boost_system-vc143-mt-x64-1_88
            boost_thread-vc143-mt-x64-1_88
            boost_filesystem-vc143-mt-x64-1_88
    )
    target_link_libraries(${RUN_NAME} PRIVATE
            "${BOOST_LIBRARYDIR}/boost_system-vc143-mt-x64-1_88.lib"
            "${BOOST_LIBRARYDIR}/boost_thread-vc143-mt-x64-1_88.lib"
            "${BOOST_LIBRARYDIR}/boost_filesystem-vc143-mt-x64-1_88.lib"
    )
else ()
    find_package(Boost REQUIRED COMPONENTS system thread filesystem)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(${RUN_NAME} PRIVATE ${Boost_LIBRARIES})
endif ()

#include(FetchContent) # it can fetch content automatically, for mingw

# Qt setting
if (APPLE)
    set(Vulkan_INCLUDE_DIR "$ENV{HOME}/VulkanSDK/1.4.321.0/macOS/include")
else ()
    set(Vulkan_INCLUDE_DIR "C:/VulkanSDK/1.4.321.1/include")
endif ()
include_directories(${Vulkan_INCLUDE_DIR})
find_package(Qt6 COMPONENTS Core Gui Widgets Multimedia REQUIRED)
message(STATUS "Qt6 found at: ${Qt6_DIR}")
# link Qt
target_link_libraries(${RUN_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Multimedia)


#-----------------------------------
# OpenMP needs to be downloaded to the mac, parallel computing api
if (APPLE)
    set(OpenMP_INCLUDE_DIR "/opt/homebrew/Cellar/libomp/20.1.8/include")
    set(OpenMP_LIBRARY "/opt/homebrew/Cellar/libomp/20.1.8/lib") # /libomp.dylib
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
        target_compile_options(${RUN_NAME} PRIVATE -fopenmp)
        target_link_options(${RUN_NAME} PRIVATE -fopenmp)
    endif ()
    if (OpenMP_CXX_FOUND)
        target_link_libraries(${RUN_NAME} PRIVATE OpenMP::OpenMP_CXX)
    endif ()
endif ()

#-----------------------------------
# matplotlib setting
if (APPLE)
    # python head file
    include_directories(/opt/anaconda3/include/python3.12)
    # numpy head file
    include_directories(/opt/anaconda3/lib/python3.12/site-packages/numpy/core/include)
    target_link_libraries(${RUN_NAME} PRIVATE
            /opt/anaconda3/lib/libpython3.12.dylib
            dl
    )
elseif (MSVC)
    include_directories(C:/ProgramData/anaconda3/include/)
    include_directories(C:/ProgramData/anaconda3/Lib/site-packages/numpy/core/include/)
    target_link_libraries(${RUN_NAME} PRIVATE
            C:/ProgramData/anaconda3/libs/python312.lib
    )
else ()
    include(FetchContent)
    FetchContent_Declare(
            matplotlib_cpp
            GIT_REPOSITORY git@github.com:lava/matplotlib-cpp.git
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

# Eigen3 setting
if (APPLE)
    find_package(Eigen3 REQUIRED)
    target_link_libraries(${RUN_NAME} PRIVATE Eigen3::Eigen)
else ()
    set(EIGEN3_INCLUDE_DIR "D:/c++Library/eigen-3.4.0")
    target_include_directories(${RUN_NAME} PRIVATE ${EIGEN3_INCLUDE_DIR})
endif ()

# spdlog setting
if (APPLE)
    find_package(spdlog)
    target_link_libraries(${RUN_NAME} PRIVATE spdlog::spdlog)
else ()
    target_include_directories(${RUN_NAME} PRIVATE "D:/c++Library/spdlog/include")
endif ()

