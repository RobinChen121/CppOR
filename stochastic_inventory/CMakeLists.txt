set(RUN_NAME sdp-inventory) # 设置运行程序的名字

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(cash)
add_subdirectory(states)
add_subdirectory(workforce)
add_subdirectory(newsvendor)

set(CMAKE_BUILD_TYPE Release) # 运行 Debug 还是 Release

#set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer -pthread")
# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${RUN_NAME}
        ../utils/PMF.cpp
        states/State.cpp
        newsvendor/newsvendor_parallel.cpp
        #        cash/CashState.cpp

)

##-----------------------------------
## Boost
#find_package(Boost REQUIRED COMPONENTS system thread filesystem)
#target_link_libraries(${RUN_NAME} ${Boost_LIBRARIES})
