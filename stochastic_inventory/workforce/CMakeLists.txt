set(RUN_NAME workforce) # 设置运行程序的名字

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_BUILD_TYPE Release) # 运行 Debug 还是 Release
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer -pthread")
# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${RUN_NAME}
        worker_state.cpp
        workforce_plan.cpp

        workforce_testing_animation.cpp
        ../../utils/pmf.cpp
        ../../utils/draw_graph.cpp
)


#-----------------------------------
# Boost
# Boost 提供了很多通用的的库，例如数值计算等
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
target_link_libraries(${RUN_NAME} ${Boost_LIBRARIES})

#-----------------------------------
# matplotlib
# 设置 Python 头文件路径
include_directories(/opt/anaconda3/include/python3.12)
# numpy 头文件路径
include_directories(/opt/anaconda3/lib/python3.12/site-packages/numpy/core/include)
# 链接所需库
target_link_libraries(${RUN_NAME}
        /opt/anaconda3/lib/libpython3.12.dylib
        dl
)