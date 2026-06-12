cmake_minimum_required(VERSION 3.30)
project(sddp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else ()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif ()

# ==================== 仅在 Debug 模式下启用 ASan ====================
if (APPLE)
    # Mac (Clang): 仅在当前配置为 Debug 时，追加 ASan 编译和链接标志
    add_compile_options("$<$<CONFIG:Debug>:-fsanitize=address;-g;-fno-omit-frame-pointer>")
    add_link_options("$<$<CONFIG:Debug>:-fsanitize=address>")

elseif (MSVC)
    # Windows (MSVC): 仅在当前配置为 Debug 时，追加微软的 ASan 编译和链接标志
    # 注意：MSVC 多个参数在生成器表达式中需要用分号 ";" 隔开
    add_compile_options("$<$<CONFIG:Debug>:/fsanitize=address;/Zi;/Oy->")

    add_executable(${PROJECT_NAME}
            #            newsvendor_backorder.cpp

            newsvendor_backorder_highs.cpp

            ../../utils/sampling.cpp
    )
endif ()

# highs setting
# 强制关闭 HiGHS 自身的测试和可执行文件编译，只编译库本身（节省 80% 的编译时间）
set(FAST_BUILD ON CACHE BOOL "Disable testing and executables for HiGHS" FORCE)
# 将本地 HiGHS 路径添加为子目录
# 参数1: 你的本地源码路径
# 参数2: 编译输出的二进制路径（写在 build 目录下以免污染源码）
add_subdirectory("D:/chenzhen/HiGHS" "${CMAKE_BINARY_DIR}/highs_build")
# 现代 CMake 推荐的一键绑定
# 这会自动处理头文件路径（Include）和库文件链接（Link）
target_link_libraries(${PROJECT_NAME} PRIVATE highs::highs)

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
    option(CXX "enable C++ compilation" ON)
    if (CXX)
        enable_language(CXX)
    endif ()

    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

    # ==================== MSVC 运行库配置 (/MT 或 /MD) ====================
    if (MSVC AND MT)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        message(STATUS "Using Static Runtime Libraries (/MT or /MTd)")
    endif ()

    # ==================== Gurobi 13.0.2 显式配置 ====================
    set(GUROBI_DIR "C:/gurobi1302/win64")
    set(ENV{GUROBI_HOME} "${GUROBI_DIR}")

    # 1. 自动定位 Gurobi 核心 C 库（gurobi130.lib，这个库不分 debug/release）
    find_library(GUROBI_LIBRARY
            NAMES gurobi130
            PATHS "${GUROBI_DIR}/lib"
            REQUIRED)

    # 2. 区分 Debug 和 Release 定位 C++ 封装库
    # 寻找 Release 版本库 (gurobi_c++md2017.lib)
    find_library(GUROBI_CXX_RELEASE
            NAMES gurobi_c++md2017
            PATHS "${GUROBI_DIR}/lib"
            REQUIRED)

    # 寻找 Debug 版本库 (gurobi_c++mdd2017.lib)
    find_library(GUROBI_CXX_DEBUG
            NAMES gurobi_c++mdd2017
            PATHS "${GUROBI_DIR}/lib"
            REQUIRED)

    # 3. 打印找库状态，方便你在控制台 debug
    message(STATUS "Gurobi include dir: ${GUROBI_DIR}/include")
    message(STATUS "Gurobi core library found at: ${GUROBI_LIBRARY}")
    message(STATUS "Gurobi C++ Release library: ${GUROBI_CXX_RELEASE}")
    message(STATUS "Gurobi C++ Debug library: ${GUROBI_CXX_DEBUG}")

    # ==================== 将 Gurobi 绑定到你的项目 ====================
    if (CXX)
        # 注入头文件路径
        target_include_directories(${PROJECT_NAME} PRIVATE "${GUROBI_DIR}/include")

        # 4. 智能链接：根据当前编译配置(optimized=Release, debug=Debug)选择库
        target_link_libraries(${PROJECT_NAME} PRIVATE
                optimized ${GUROBI_CXX_RELEASE}
                debug ${GUROBI_CXX_DEBUG}
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