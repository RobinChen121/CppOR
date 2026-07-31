cmake_minimum_required(VERSION 3.30)
project(random_yield) # 设置运行程序的名字

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
        two_product.cpp
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
