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
# boost setting
if(NOT APPLE)
    # 假设 Boost 安装在 C:/local/boost_1_88_0
    set(BOOST_ROOT "C:/local/boost_1_88_0")
    set(BOOST_INCLUDEDIR "${BOOST_ROOT}")
    set(BOOST_LIBRARYDIR "${BOOST_ROOT}/stage/lib")
    # 添加头文件目录
    include_directories(${BOOST_INCLUDEDIR})
    # 添加库目录
    link_directories(${BOOST_LIBRARYDIR})
    # 指定要链接的 Boost 静态库（或动态库）
    # 注意：这些库名要与你 stage/lib 目录下的 .lib 文件一致（无需 .lib 后缀）
    set(BOOST_LIBS
            boost_system-vc143-mt-x64-1_88
            boost_thread-vc143-mt-x64-1_88
            boost_filesystem-vc143-mt-x64-1_88
    )
    target_link_libraries(${RUN_NAME} PRIVATE
            "${BOOST_LIBRARYDIR}/boost_system-vc143-mt-x64-1_88.lib"
            "${BOOST_LIBRARYDIR}/boost_thread-vc143-mt-x64-1_88.lib"
            "${BOOST_LIBRARYDIR}/boost_filesystem-vc143-mt-x64-1_88.lib"
    )
else ()
    find_package(Boost REQUIRED COMPONENTS system thread filesystem)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(${RUN_NAME} PRIVATE ${Boost_LIBRARIES})
endif()


#-----------------------------------
# 下面为调用 matplotlibcpp 的设置
if (APPLE)
    # 设置 Python 头文件路径
    include_directories(/opt/anaconda3/include/python3.12)
    # numpy 头文件路径
    include_directories(/opt/anaconda3/lib/python3.12/site-packages/numpy/core/include)
    # 链接所需库
    target_link_libraries(${RUN_NAME} PRIVATE
            /opt/anaconda3/lib/libpython3.12.dylib
            dl
    )
else ()
    FetchContent_Declare(
            matplotlib_cpp
            GIT_REPOSITORY https://github.com/lava/matplotlib-cpp.git
            GIT_TAG master
    )
    FetchContent_MakeAvailable(matplotlib_cpp)
    # Include matplotlib-cpp
    target_include_directories(${RUN_NAME} PRIVATE ${matplotlib_cpp_SOURCE_DIR})
    # Link Python and NumPy
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
    target_include_directories(${RUN_NAME} PRIVATE
            ${Python3_INCLUDE_DIRS}
            ${Python3_NumPy_INCLUDE_DIRS}
    )
    target_link_libraries(${RUN_NAME} PRIVATE
            ${Python3_LIBRARIES}
    )
endif ()

