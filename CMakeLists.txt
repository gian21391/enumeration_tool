cmake_minimum_required(VERSION 3.10)
project(enumerator_test)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(lib)

add_library(enumeration_tool INTERFACE)
target_include_directories(enumeration_tool INTERFACE include)
target_link_libraries(enumeration_tool INTERFACE nauty kitty fmt iterator_tpl magic_enum json)
target_compile_definitions(enumeration_tool INTERFACE)

add_subdirectory(examples)
add_subdirectory(test)
add_subdirectory(benchmark)
add_subdirectory(experiments)