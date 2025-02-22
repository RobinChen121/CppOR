cmake_minimum_required(VERSION 3.30)
project(CppClion)

set(CMAKE_CXX_STANDARD 20)

# 如果 Boost 安装在非标准路径，手动设置 BOOST_ROOT 和相关变量
set(BOOST_ROOT "/opt/homebrew/Cellar/boost/1.87.0")  # 根据实际路径修改，比如 /usr/local 对于 Intel 机器
set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include")
set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib")

# 查找 Boost（如果只需要头文件库，可以不指定 COMPONENTS）
find_package(Boost REQUIRED COMPONENTS filesystem system)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    message(STATUS "Boost found: ${Boost_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "Could not find Boost!")
endif()

add_executable(${PROJECT_NAME} main.cpp boost_folder/boost_test.cpp)

target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

