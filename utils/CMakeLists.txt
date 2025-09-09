set(RUN_NAME utils) # 设置运行程序的名字

# 创建可执行程序，需要运行的 cpp 文件放在这里
add_executable(${RUN_NAME}
        #        gurobi_test.cpp
        pmf.cpp
        #        chen_solver/Var.cpp
        #        chen_solver/Model.cpp
)

## boost setting
if (MSVC)
    set(BOOST_ROOT "C:/local/boost_1_88_0")
    set(BOOST_INCLUDEDIR "${BOOST_ROOT}")
    set(BOOST_LIBRARYDIR "${BOOST_ROOT}/stage/lib")
    include_directories(${BOOST_INCLUDEDIR})
    link_directories(${BOOST_LIBRARYDIR})
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
endif ()


#-----------------------------------
# matplotlib setting
if (APPLE)
    # python head file
    include_directories(/opt/anaconda3/include/python3.12)
    # numpy head file
    include_directories(/opt/anaconda3/lib/python3.12/site-packages/numpy/core/include)
    target_link_libraries(${RUN_NAME} PRIVATE
            /opt/anaconda3/lib/libpython3.12.dylib
            dl
    )
elseif (MSVC)
    include_directories(C:/ProgramData/anaconda3/include/)
    include_directories(C:/ProgramData/anaconda3/Lib/site-packages/numpy/core/include/)
    target_link_libraries(${RUN_NAME} PRIVATE
            C:/ProgramData/anaconda3/libs/python312.lib
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

