cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

set(RUN_NAME qt_practice) # the name of configuration target
set(CMAKE_AUTOMOC ON)
add_executable(${RUN_NAME}
        tetriswidget.cpp
        main.cpp)

# Qt setting
if (APPLE)
    set(Vulkan_INCLUDE_DIR "$ENV{HOME}/VulkanSDK/1.4.313.1/macOS/include")
else ()
    set(Vulkan_INCLUDE_DIR "C:/VulkanSDK/1.4.321.1/include")
endif ()
include_directories(${Vulkan_INCLUDE_DIR})
find_package(Qt6 COMPONENTS Core Gui Widgets Multimedia REQUIRED)
message(STATUS "Qt6 found at: ${Qt6_DIR}")
# link Qt
target_link_libraries(${RUN_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Multimedia)