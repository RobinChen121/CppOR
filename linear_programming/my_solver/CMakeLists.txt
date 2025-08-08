set(RUN_NAME my_solver) # 设置运行程序的名字

set(CMAKE_BUILD_TYPE Release)
if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()
# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${RUN_NAME}
        simplex.cpp
)