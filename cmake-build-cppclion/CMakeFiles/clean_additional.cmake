# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "junior_practices/qt_practices/CMakeFiles/qt_practice_autogen.dir/AutogenUsed.txt"
  "junior_practices/qt_practices/CMakeFiles/qt_practice_autogen.dir/ParseCache.txt"
  "junior_practices/qt_practices/qt_practice_autogen"
  )
endif()
