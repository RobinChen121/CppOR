cmake_minimum_required(VERSION 3.30)
Project(workforce) # 设置运行程序的名字

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(CMAKE_BUILD_TYPE Debug) # 运行 Debug 还是 Release
if (CMAKE_BUILD_TYPE STREQUAL Release)
    if (MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "/Ox")
    else ()
        set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    endif ()
endif ()

#if (MSVC)
#    # 开启 AddressSanitizer, 数组越界检查
#    # 需要将 clang_rt.asan_dynamic-x86_64.dll 这个文件地址添加到系统 path
#    # 或将文件拷贝到当前项目 exe 所在文件夹
#    if (CMAKE_BUILD_TYPE STREQUAL Debug)
#        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Od") # Od
#    endif ()
#endif ()

# set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer -pthread")
# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${PROJECT_NAME}
        worker_state.cpp
        workforce_plan_new.cpp
        #        workforce_plan.cpp

        #        workforce_testing_animation.cpp
        #        ../../utils/pmf.cpp
        ../../utils/common.h
        #        ../../utils/draw_graph.cpp
        #        binomial_product_test.cpp
        #        Binomial_conditional_expectation_test.cpp
        #        Binomial_stochastic_order_test.cpp
        #        Binomial_probability_decomposition_test.cpp
        #        Binomial_logconcave_test.cpp
        piecewise.cpp
        util_binomial.cpp
)

## boost setting
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
    find_package(Boost REQUIRED COMPONENTS thread filesystem)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
endif ()

## 设置 Gurobi 路径（根据你的安装调整）
if (APPLE)
    set(GUROBI_HOME "/Library/gurobi1302/macos_universal2") # gurobi 的安装地址
    set(GUROBI_INCLUDE_DIR "${GUROBI_HOME}/include") # 头文件路径
    set(GUROBI_LIB_DIR "${GUROBI_HOME}/lib") # 库文件路径
    # 将路径添加到搜索地址
    #    include_directories(${GUROBI_INCLUDE_DIR}) # include_directories() 是过时的命令，建议使用 target_include_directories() 来指定目标的包含目录
    #    include_directories(${GUROBI_LIB_DIR})
    # target_link_libraries 必须使用关键字 PRIVATE 或 PUBLIC 来指定链接库的可见性
    target_include_directories(${PROJECT_NAME} PRIVATE ${GUROBI_INCLUDE_DIR}) # 添加 Gurobi 的头文件路径到编译器的搜索路径
    target_include_directories(${PROJECT_NAME} PRIVATE ${GUROBI_LIB_DIR})
    target_link_libraries(${PROJECT_NAME}
            "${GUROBI_LIB_DIR}/libgurobi_c++.a" # .a 是静态库
            "${GUROBI_LIB_DIR}/libgurobi130.dylib" # .dylib 是动态库
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

#-----------------------------------
# OpenMP needs to be downloaded to the mac, parallel computing api
if (APPLE)
    set(OpenMP_INCLUDE_DIR "/opt/homebrew/Cellar/libomp/21.1.8/include")
    set(OpenMP_LIBRARY "/opt/homebrew/Cellar/libomp/21.1.8/lib") # /libomp.dylib
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
        target_compile_options(${PROJECT_NAME} PRIVATE /openmp)
    else ()
        target_compile_options(${PROJECT_NAME} -fopenmp)
        target_link_options(${PROJECT_NAME} -fopenmp)
    endif ()
    if (OpenMP_CXX_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
    endif ()
endif ()

##-----------------------------------
## matplotlib setting
#if (APPLE)
#    # python head file
#    include_directories(/opt/anaconda3/include/python3.13)
#    # numpy head file
#    include_directories(/opt/anaconda3/lib/python3.13/site-packages/numpy/_core/include)
#    target_link_libraries(${PROJECT_NAME}
#            /opt/anaconda3/lib/libpython3.13.dylib
#            dl
#    )
#elseif (MSVC)
#    find_package(Python3 REQUIRED COMPONENTS Development NumPy)
#
#    target_include_directories(${PROJECT_NAME} PRIVATE
#            ${Python3_INCLUDE_DIRS}
#            ${Python3_NumPy_INCLUDE_DIRS}
#    )
#
#    target_link_libraries(${PROJECT_NAME} PRIVATE
#            ${Python3_LIBRARIES}
#    )
#else ()
#    FetchContent_Declare(
#            matplotlib_cpp
#            GIT_REPOSITORY https://github.com/lava/matplotlib-cpp.git
#            GIT_TAG master
#    )
#    FetchContent_MakeAvailable(matplotlib_cpp)
#    # Include matplotlib-cpp
#    target_include_directories(${PROJECT_NAME} ${matplotlib_cpp_SOURCE_DIR})
#    # Link Python and NumPy
#    find_package(Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
#    target_include_directories(${PROJECT_NAME}
#            ${Python3_INCLUDE_DIRS}
#            ${Python3_NumPy_INCLUDE_DIRS}
#    )
#    target_link_libraries(${PROJECT_NAME} PRIVATE
#            ${Python3_LIBRARIES}
#    )
#endif ()