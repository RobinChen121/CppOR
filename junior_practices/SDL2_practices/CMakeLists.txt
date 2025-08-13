cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)
set(RUN_NAME sdl_practice) # the name of configuration target


if (MSVC)
    add_compile_options(/utf-8) # 增加中文文字编码
endif ()
add_executable(${RUN_NAME}
        tetris_main.cpp
        Tetris.cpp
        Color.cpp
        Piece.cpp

        #        array_boundary_test.cpp
)

# sdl2 game setting
if (APPLE)
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
    target_link_libraries(${RUN_NAME} PRIVATE
            ${SDL2_LIBRARIES}
    )
else ()
    # SDL2 路径
    set(SDL2_PATH "C:/SDL2-2.30.10/")  # 改成你自己的路径
    # 包含头文件
    include_directories(${SDL2_PATH}/include)
    # 链接库目录
    link_directories(${SDL2_PATH}/lib/x64)
    # 链接 SDL2
    target_link_libraries(${RUN_NAME}
            C:/SDL2-2.30.10/lib/x64/SDL2.lib
            C:/SDL2-2.30.10/lib/x64/SDL2main.lib)
endif ()