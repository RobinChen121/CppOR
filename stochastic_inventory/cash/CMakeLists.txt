set(RUN_NAME cash-sdp) # 设置运行程序的名字


set(CMAKE_BUILD_TYPE Debug) # 运行 Debug 还是 Release

#set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer -pthread")
# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${RUN_NAME}
        ../../utils/PMF.cpp

        #        ../states/State.cpp
        #        ../states/CashState.cpp
        #        ../states/CashLeadtimeState.cpp

        ../../utils/selfHash.cpp

        ../states/CashLeadtimeMultiState.cpp
        overdraftLeadtimeDoubleProduct.cpp
)

#-----------------------------------
# Boost
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
target_link_libraries(${RUN_NAME} ${Boost_LIBRARIES})