# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "D:/chenzhen/CppOR/junior_practices/cmake-build-debug/_deps/matplotlib_cpp-src")
  file(MAKE_DIRECTORY "D:/chenzhen/CppOR/junior_practices/cmake-build-debug/_deps/matplotlib_cpp-src")
endif()
file(MAKE_DIRECTORY
  "D:/chenzhen/CppOR/junior_practices/cmake-build-debug/_deps/matplotlib_cpp-build"
  "D:/chenzhen/CppOR/junior_practices/cmake-build-debug/_deps/matplotlib_cpp-subbuild/matplotlib_cpp-populate-prefix"
  "D:/chenzhen/CppOR/junior_practices/cmake-build-debug/_deps/matplotlib_cpp-subbuild/matplotlib_cpp-populate-prefix/tmp"
  "D:/chenzhen/CppOR/junior_practices/cmake-build-debug/_deps/matplotlib_cpp-subbuild/matplotlib_cpp-populate-prefix/src/matplotlib_cpp-populate-stamp"
  "D:/chenzhen/CppOR/junior_practices/cmake-build-debug/_deps/matplotlib_cpp-subbuild/matplotlib_cpp-populate-prefix/src"
  "D:/chenzhen/CppOR/junior_practices/cmake-build-debug/_deps/matplotlib_cpp-subbuild/matplotlib_cpp-populate-prefix/src/matplotlib_cpp-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/chenzhen/CppOR/junior_practices/cmake-build-debug/_deps/matplotlib_cpp-subbuild/matplotlib_cpp-populate-prefix/src/matplotlib_cpp-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/chenzhen/CppOR/junior_practices/cmake-build-debug/_deps/matplotlib_cpp-subbuild/matplotlib_cpp-populate-prefix/src/matplotlib_cpp-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
