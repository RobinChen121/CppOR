set(RUN_NAME sddp-overdraft) # 设置运行程序的名字


set(CMAKE_BUILD_TYPE Release)


# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${RUN_NAME}
        double_product_testing.cpp
        double_product_enhancement_further.cpp

        #        single_product_testing_iteratonNum.cpp
        #        single_product_enhancement.cpp
        I_cash_status.cpp
        ../../utils/common.cpp
        ../../utils/sampling.cpp
)

# 设置 Gurobi 路径（根据你的安装调整）
if(apple)
    set(GUROBI_HOME "/Library/gurobi1201/macos_universal2") # gurobi 的安装地址
else ()
    set(GUROBI_HOME "C:/gurobi1202/win64") # gurobi 的安装地址
endif ()
set(GUROBI_INCLUDE_DIR "${GUROBI_HOME}/include") # 头文件路径
set(GUROBI_LIB_DIR "${GUROBI_HOME}/lib") # 库文件路径

# 将路径添加到搜索地址
include_directories(${GUROBI_LIB_DIR})
include_directories(${GUROBI_INCLUDE_DIR})

# 链接 Gurobi 库，这个静态库与动态库都需要
target_link_libraries(${RUN_NAME}
        "${GUROBI_LIB_DIR}/libgurobi_c++.a" # .a 是静态库
        "${GUROBI_LIB_DIR}/libgurobi120.dylib" # .dylib 是动态库
)

find_package(Boost REQUIRED COMPONENTS system thread filesystem)
target_link_libraries(${RUN_NAME} ${Boost_LIBRARIES})