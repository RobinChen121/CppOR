set(RUN_NAME utils) # 设置运行程序的名字

# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${RUN_NAME}
        #        gurobi_test.cpp
        PMF.cpp
        #        my_solver/Var.cpp
        #        my_solver/Model.cpp
)

#-----------------------------------
# Boost
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
target_link_libraries(${RUN_NAME} PRIVATE ${Boost_LIBRARIES})


#-----------------------------------
# matplotlib
# 设置 Python 头文件路径
include_directories(/opt/anaconda3/include/python3.12)
# numpy 头文件路径
include_directories(/opt/anaconda3/lib/python3.12/site-packages/numpy/core/include)
# 链接所需库
target_link_libraries(${RUN_NAME} PRIVATE
        /opt/anaconda3/lib/libpython3.12.dylib
        dl
)

