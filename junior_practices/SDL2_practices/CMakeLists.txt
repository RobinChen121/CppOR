cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

Project(sdl_practice) # the name of configuration target


set(CMAKE_BUILD_TYPE Release) # 运行 Debug 还是 Release
# 强制使用 -O3（你也可以改为 -O2）
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    if (MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "/O2")
    else ()
        set(CMAKE_CXX_FLAGS_RELEASE "-O2")
    endif ()
endif ()

if (MSVC)
    add_compile_options(/utf-8) # 增加中文文字编码
endif ()
add_executable(${PROJECT_NAME}
        tetris_main.cpp
        Tetris.cpp
        Piece.cpp

        #        array_boundary_test.cpp
)

# sdl2 game setting
if (APPLE)
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
    target_link_libraries(${RROJECT_NAME} PRIVATE
            ${SDL2_LIBRARIES}
    )
else ()
    # SDL2 路径
    set(SDL2_PATH "C:/SDL2-2.30.10/")  # 改成你自己的路径
    # 包含头文件
    include_directories(${SDL2_PATH}/include)
#    # 链接库目录
#    link_directories(${SDL2_PATH}/lib/x64)

    # SDL2_mixer 手动设置
    set(SDL2_MIXER_INCLUDE_DIRS "C:/SDL2_mixer-2.8.1/include")
    include_directories(${SDL2_MIXER_INCLUDE_DIRS})
    # 链接库
    set(SDL2_MIXER_LIBRARIES "C:/SDL2_mixer-2.8.1/lib/x64/SDL2_mixer.lib")

    # SDL2_ttf 手动设置
    set(SDL2_TTF_INCLUDE_DIRS "C:/SDL2_ttf-2.24.0/include")
    include_directories(${SDL2_TTF_INCLUDE_DIRS})
    # 链接库
    set(SDL2_TTF_LIBRARIES "C:/SDL2_ttf-2.24.0/lib/x64/SDL2_ttf.lib")

    # 链接 SDL2, SDL2_mixer
    target_link_libraries(${PROJECT_NAME}
            C:/SDL2-2.30.10/lib/x64/SDL2.lib
            C:/SDL2-2.30.10/lib/x64/SDL2main.lib
            ${SDL2_MIXER_LIBRARIES}
            ${SDL2_TTF_LIBRARIES}
    )
endif ()