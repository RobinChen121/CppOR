set(RUN_NAME LP) # 设置运行程序的名字

add_subdirectory(sddp_overdraft)
add_subdirectory(gurobi_examples)

set(CMAKE_BUILD_TYPE Release)
if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else ()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif ()
# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${RUN_NAME}
        gurobi_test.cpp
        #        gurobi_examples/mip1_c++.cpp
        #        chen_solver/Var.cpp
        #        chen_solver/Model.cpp
)

### 设置 Gurobi 路径（根据你的安装调整）
if (APPLE)
    set(GUROBI_HOME "/Library/gurobi1201/macos_universal2") # gurobi 的安装地址
    set(GUROBI_INCLUDE_DIR "${GUROBI_HOME}/include") # 头文件路径
    set(GUROBI_LIB_DIR "${GUROBI_HOME}/lib") # 库文件路径
    # 将路径添加到搜索地址
    include_directories(${GUROBI_INCLUDE_DIR})
    include_directories(${GUROBI_LIB_DIR})
    target_link_libraries(${RUN_NAME}
            PRIVATE
            "${GUROBI_LIB_DIR}/libgurobi_c++.a" # .a 是静态库
            "${GUROBI_LIB_DIR}/libgurobi120.dylib" # .dylib 是动态库
    )
else ()
    option(CXX "enable C++ compilation" ON)  # 定义一个选项 CXX，描述为 "enable C++ compilation"，默认值为 ON（开启）
    if (CXX)
        enable_language(CXX)  # 如果 CXX 选项被开启，告诉 CMake 需要启用 C++ 语言支持（使用 C++ 编译器）
    endif ()
    set(CMAKE_MODULE_PATH "C:/gurobi1202/win64") # this is find package path
    set(GUROBI_DIR "C:/gurobi1202/win64")
    set(ENV{GUROBI_HOME} "${GUROBI_DIR}")

    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

    # Visual Studio compiler with static runtime libraries
    if (MSVC AND MT)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
    endif ()
    # 通过 FindGUROBI.cmake 查找 Gurobi
    find_package(GUROBI 120 REQUIRED)
    include_directories(${GUROBI_INCLUDE_DIRS})
    include_directories(${GUROBI_CXX_LIBRARY})
    message(STATUS "Gurobi include dirs: ${GUROBI_INCLUDE_DIRS}")
    message(STATUS "Gurobi libraries: ${GUROBI_CXX_LIBRARY}")

    if (CXX)
        set(CMAKE_CXX_STANDARD 11)
        target_link_libraries(${RUN_NAME} optimized ${GUROBI_CXX_LIBRARY}
                debug ${GUROBI_CXX_DEBUG_LIBRARY})
    endif ()

    target_link_libraries(${RUN_NAME} ${GUROBI_LIBRARY})

    if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
        include(FeatureSummary)
        feature_summary(WHAT ALL)
    endif ()
endif ()