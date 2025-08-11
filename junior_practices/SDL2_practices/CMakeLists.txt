cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Release)
set(RUN_NAME sdl_practice) # the name of configuration target


add_executable(${RUN_NAME}
        sdl2_test.cpp
)

# sdl2 game setting
if (APPLE)
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
    target_link_libraries(${RUN_NAME} PRIVATE
            ${SDL2_LIBRARIES}
    )
endif ()