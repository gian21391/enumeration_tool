cmake_minimum_required(VERSION 3.10)
project(enumerator_test)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(lib)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TBBGet.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TBBBuild.cmake)
tbb_get(TBB_ROOT tbb_root SOURCE_CODE)
tbb_build(TBB_ROOT ${tbb_root} CONFIG_DIR TBB_DIR)
find_package(TBB REQUIRED tbb)

add_definitions(-DPERFORMANCE_MONITORING -DTBB_USE_GLIBCXX_VERSION=70400)

add_library(enumeration_tool INTERFACE)
target_include_directories(enumeration_tool INTERFACE include)
target_link_libraries(enumeration_tool INTERFACE nauty kitty fmt iterator_tpl magic_enum json range-v3 ${TBB_IMPORTED_TARGETS})
target_compile_definitions(enumeration_tool INTERFACE)

#add_subdirectory(examples)
add_subdirectory(test)
add_subdirectory(experiments)