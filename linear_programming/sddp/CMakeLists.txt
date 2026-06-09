cmake_minimum_required(VERSION 3.30)
project(sddp)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else ()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif ()

add_executable(${PROJECT_NAME}
        newsvendor_backorder.cpp

        ../../utils/sampling.cpp
)

### Gurobi settings
if (APPLE)
    set(GUROBI_HOME "/Library/gurobi1201/macos_universal2")
    set(GUROBI_INCLUDE_DIR "${GUROBI_HOME}/include") # header folder
    set(GUROBI_LIB_DIR "${GUROBI_HOME}/lib") # lib folder

    include_directories(${GUROBI_INCLUDE_DIR})
    include_directories(${GUROBI_LIB_DIR})
    target_link_libraries(${PROJECT_NAME}

            "${GUROBI_LIB_DIR}/libgurobi_c++.a" # .a is static lib
            "${GUROBI_LIB_DIR}/libgurobi120.dylib" # .dylib is dynamic lib
    )
else ()
    # ==================== C++ 语言启用 ====================
    option(CXX "enable C++ compilation" ON)
    if (CXX)
        enable_language(CXX)
    endif ()

    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

    # ==================== MSVC 运行库配置 (/MT 或 /MD) ====================
    # 提示：Gurobi 官方默认提供的是 /MD（动态链接运行时）对应的库。
    # 如果你强制使用 /MT（静态链接），可能会遇到符号冲突。以下保留你的逻辑并做规范：
    if (MSVC AND MT)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        message(STATUS "Using Static Runtime Libraries (/MT or /MTd)")
    endif ()

    # ==================== Gurobi 13.0.2 显式配置 ====================
    set(GUROBI_DIR "C:/gurobi1302/win64")
    set(ENV{GUROBI_HOME} "${GUROBI_DIR}")

    # 1. 自动定位 Gurobi 核心 C 库（gurobi130.lib）
    find_library(GUROBI_LIBRARY
            NAMES gurobi130
            PATHS "${GUROBI_DIR}/lib"
            REQUIRED)

    # 2. 自动定位 Gurobi C++ 封装库（gurobi_c++md2017.lib）
    find_library(GUROBI_CXX_LIBRARY
            NAMES gurobi_c++md2017
            PATHS "${GUROBI_DIR}/lib"
            REQUIRED)

    # 3. 打印找库状态，方便你在控制台 debug
    message(STATUS "Gurobi include dir: ${GUROBI_DIR}/include")
    message(STATUS "Gurobi core library found at: ${GUROBI_LIBRARY}")
    message(STATUS "Gurobi C++ library found at: ${GUROBI_CXX_LIBRARY}")

    # ==================== 将 Gurobi 绑定到你的项目 ====================
    if (CXX)
        # 注入头文件路径
        target_include_directories(${PROJECT_NAME} PRIVATE "${GUROBI_DIR}/include")

        # 链接 Gurobi 的 C 和 C++ 库
        target_link_libraries(${PROJECT_NAME} PRIVATE
                ${GUROBI_CXX_LIBRARY}
                ${GUROBI_LIBRARY}
        )
    endif ()

    # ==================== 功能摘要（保留你的原有逻辑） ====================
    if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
        include(FeatureSummary)
        feature_summary(WHAT ALL)
    endif ()
endif ()

#find_package(Boost REQUIRED COMPONENTS system thread filesystem)
#target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})