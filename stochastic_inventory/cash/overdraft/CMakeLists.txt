set(RUN_NAME sdp-overdraft) # 设置运行程序的名字


set(CMAKE_BUILD_TYPE Release) # 运行 Debug 还是 Release

#set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer -pthread")
# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${RUN_NAME}
        ../../../utils/PMF.cpp
        ../../../utils/common.cpp

        ../../states/State.cpp
        ../../states/cash_state.cpp
        ../../states/cash_leadtime_state.cpp

        #        ../../states/CashLeadtimeMultiState.cpp
        leadtime_single_product.cpp

        #        leadtime_single_product_testing.cpp

)

#-----------------------------------
# Boost
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
target_link_libraries(${RUN_NAME} ${Boost_LIBRARIES})