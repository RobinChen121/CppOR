# Install script for directory: D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/CppClion")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "C:/Program Files/JetBrains/CLion 2024.3.5/bin/mingw/bin/objdump.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/eigen3/unsupported/Eigen" TYPE FILE FILES
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/AdolcForward"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/AlignedVector3"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/ArpackSupport"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/AutoDiff"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/BVH"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/EulerAngles"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/FFT"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/IterativeSolvers"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/KroneckerProduct"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/LevenbergMarquardt"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/MatrixFunctions"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/MoreVectorization"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/MPRealSupport"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/NonLinearOptimization"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/NumericalDiff"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/OpenGLSupport"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/Polynomials"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/Skyline"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/SparseExtra"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/SpecialFunctions"
    "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/Splines"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/eigen3/unsupported/Eigen" TYPE DIRECTORY FILES "D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-src/unsupported/Eigen/src" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/chenzhen/CppOR/cmake-build-debug/_deps/eigen-build/unsupported/Eigen/CXX11/cmake_install.cmake")

endif()

