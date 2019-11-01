cmake_minimum_required(VERSION 3.10)
project(enumerator_test)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(lib)

add_library(enumeration_tool INTERFACE)
target_include_directories(enumeration_tool INTERFACE include)
target_link_libraries(enumeration_tool INTERFACE nauty kitty iterator_tpl)

add_subdirectory(examples)
add_subdirectory(test)
add_subdirectory(benchmark)