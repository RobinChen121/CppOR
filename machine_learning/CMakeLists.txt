cmake_minimum_required(VERSION 3.30)
project(machine_learning) # 设置运行程序的名字

## 假设 data 文件夹和 CMakeLists.txt 在同一层级
file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else ()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif ()

# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${PROJECT_NAME}
        #        back_propagation.cpp
        activation.h
        #        simple_rnn.cpp
        #        rnn.cpp
        simple_nn.cpp
        #        rnn_libtorch.cpp
)

if (apple)
    set(CMAKE_PREFIX_PATH "/Users/zhenchen/libtorch")  # LibTorch 解压路径
    find_package(Torch REQUIRED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
    target_link_libraries(${PROJECT_NAME} "${TORCH_LIBRARIES}")
endif ()

#-----------------------------------
# matplotlib setting
if (APPLE)
    # python head file
    include_directories(/opt/anaconda3/envs/py312/include/python3.12)
    # numpy head file
    include_directories(/opt/anaconda3/envs/py312/lib/python3.12/site-packages/numpy/_core/include)
    target_link_libraries(${PROJECT_NAME}
            /opt/anaconda3/envs/py312/lib/libpython3.12.dylib
            dl
    )
elseif (MSVC)
    include_directories(C:/ProgramData/anaconda3/include/)
    include_directories(C:/ProgramData/anaconda3/Lib/site-packages/numpy/core/include/)
    target_link_libraries(${PROJECT_NAME}
            C:/ProgramData/anaconda3/libs/python312.lib
    )
endif ()
