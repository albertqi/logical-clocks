cmake_minimum_required(VERSION 3.17)
project(LogicalClocks)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

add_executable(process process.cpp)
add_executable(test test.cpp)