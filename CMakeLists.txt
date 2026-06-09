cmake_minimum_required(VERSION 3.30)
project(CppClion)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# ==================== 仅在 Debug 模式下启用 ASan ====================
if (APPLE)
    # Mac (Clang): 仅在当前配置为 Debug 时，追加 ASan 编译和链接标志
    add_compile_options("$<$<CONFIG:Debug>:-fsanitize=address;-g;-fno-omit-frame-pointer>")
    add_link_options("$<$<CONFIG:Debug>:-fsanitize=address>")

elseif (MSVC)
    # Windows (MSVC): 仅在当前配置为 Debug 时，追加微软的 ASan 编译和链接标志
    # 注意：MSVC 多个参数在生成器表达式中需要用分号 ";" 隔开
    add_compile_options("$<$<CONFIG:Debug>:/fsanitize=address;/Zi;/Oy->")
    add_link_options("$<$<CONFIG:Debug>:/fsanitize=address>")
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