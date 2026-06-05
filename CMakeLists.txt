cmake_minimum_required(VERSION 3.30)
project(CppClion)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# 为 Mac 上的 Clang 编译器添加 ASan 标志
if (APPLE)
    add_compile_options(-fsanitize=address -g -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif ()

add_executable(${PROJECT_NAME} main.cpp)

## 添加子文件夹
#add_subdirectory(linear_programming) # 告诉 CMake 在小括号那目录下查找 CMakeLists.txt
#add_subdirectory(junior_practices)
#add_subdirectory(stochastic_inventory)
#add_subdirectory(utils)


#set(CMAKE_BUILD_TYPE Debug) # 运行 Debug 还是 Release
## 强制使用 -O3（你也可以改为 -O2）
#if (CMAKE_BUILD_TYPE STREQUAL "Release")
#    if (MSVC)
#        set(CMAKE_CXX_FLAGS_RELEASE "/O2")
#    else ()
#        set(CMAKE_CXX_FLAGS_RELEASE "-O2")
#    endif ()
#endif ()