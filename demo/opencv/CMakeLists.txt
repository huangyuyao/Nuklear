cmake_minimum_required(VERSION 2.8)

project(nuklear_cv)

find_package(OpenCV REQUIRED)

include_directories(.)
include_directories(${OpenCV_DIR})
add_executable(nuklear_cv_demo demo.cc)
target_link_libraries(nuklear_cv_demo ${OpenCV_LIBS})
