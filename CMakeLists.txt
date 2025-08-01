cmake_minimum_required(VERSION 3.30)
project(CppClion)

# 使用 Conan 生成的 CMake 配置
#set(CMAKE_TOOLCHAIN_FILE ${CMAKE_BINARY_DIR}/build/conan_toolchain.cmake)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加子文件夹
add_subdirectory(linear_programming) # 告诉 CMake 在小括号那目录下查找 CMakeLists.txt
add_subdirectory(junior_practices)
add_subdirectory(stochastic_inventory)
add_subdirectory(utils)

if (MSVC)
    # 开启 AddressSanitizer, 数组越界检查
    # 需要将 clang_rt.asan_dynamic-x86_64.dll 这个文件地址添加到系统 path
    # 或将文件拷贝到当前项目 exe 所在文件夹
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g -O1")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
endif ()

set(CMAKE_BUILD_TYPE Debug) # 运行 Debug 还是 Release
# 强制使用 -O3（你也可以改为 -O2）
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    if (MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "/O2")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "-O2")
    endif()
endif ()

#-----------------------------------
add_executable(${PROJECT_NAME}
        main.cpp
        #        stochastic_inventory/NewsvendorC.c

)  # no need to put .h file here

#if(APPLE)
#    find_package(Eigen3 REQUIRED)
#    target_link_libraries(${PROJECT_NAME} Eigen3::Eigen)
#else()
#    set(EIGEN3_INCLUDE_DIR "D:/c++Library/eigen-3.4.0")
#    target_include_directories(${PROJECT_NAME} PRIVATE ${EIGEN3_INCLUDE_DIR})
#endif()


#------------------------
# 如果 Boost 安装在非标准路径，手动设置 BOOST_ROOT 和相关变量
#set(BOOST_ROOT "/opt/homebrew/Cellar/boost/1.87.0")  # 根据实际路径修改，比如 /usr/local 对于 Intel 机器
#set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include")
#set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib")
## 查找 Boost（如果只需要头文件库，可以不指定 COMPONENTS）
#find_package(Boost REQUIRED COMPONENTS filesystem system)
#if (Boost_FOUND)
#    include_directories(${Boost_INCLUDE_DIRS})
#    message(STATUS "Boost found: ${Boost_INCLUDE_DIRS}")
#else ()
#    message(FATAL_ERROR "Could not find Boost!")
#endif ()
#target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})


#-----------------------------------
## 查找 Pybind11, 执行 python 代码，很鸡肋，只能弄一些简单的，比较复杂
## 查找 JKQTPlotter, 无法运行


