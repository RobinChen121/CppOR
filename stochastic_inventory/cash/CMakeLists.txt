cmake_minimum_required(VERSION 3.30)
project(sdp-cash)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else ()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif ()

#set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer -pthread")
# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${PROJECT_NAME}
        ../../utils/PMF.cpp

        ../states/State.cpp
        ../states/cash_state.h
        ../states/cash_state_newhash.h
        strong_constraint.cpp
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

