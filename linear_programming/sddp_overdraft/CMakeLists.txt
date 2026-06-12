cmake_minimum_required(VERSION 3.30)
project(sddp-overdraft) # 设置运行程序的名字

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else ()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif ()

# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${PROJECT_NAME}
        #        double_product_enhancement_further.cpp
        #        double_product_testing.cpp
        #        double_product_enhancement_further.cpp

        #        single_product_enhancement_further.cpp

        I_cash_status.cpp

        #        single_product_testing_iteration_num.cpp
        #        single_product_enhancement.cpp
        single_product.cpp
        #        I_cash_status.cpp

        ../../utils/common.cpp
        ../../utils/sampling.cpp
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
    target_link_libraries(${PROJECT_NAME} PRIVATE
            "${BOOST_LIBRARYDIR}/boost_system-vc143-mt-x64-1_88.lib"
            "${BOOST_LIBRARYDIR}/boost_thread-vc143-mt-x64-1_88.lib"
            "${BOOST_LIBRARYDIR}/boost_filesystem-vc143-mt-x64-1_88.lib"
    )
else ()
    set(BOOST_ROOT "/opt/homebrew/Cellar/boost/1.90.0")
    set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include")
    set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib")
    include_directories(${BOOST_INCLUDEDIR})
    link_directories(${BOOST_LIBRARYDIR})
endif ()

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