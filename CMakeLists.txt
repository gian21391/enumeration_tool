cmake_minimum_required(VERSION 3.14)
project(enumerator_test)

set(CMAKE_CXX_STANDARD 17)

add_library(enumeration_tool INTERFACE)
target_include_directories(enumeration_tool INTERFACE include)

add_subdirectory(lib)
add_subdirectory(examples)
