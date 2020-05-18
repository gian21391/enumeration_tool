cmake_minimum_required(VERSION 3.10)
project(enumerator_test)

set(CMAKE_CXX_STANDARD 17)

#include(CheckCXXCompilerFlag)
#CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
#if(COMPILER_SUPPORTS_MARCH_NATIVE)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
#endif()
#CHECK_CXX_COMPILER_FLAG("-mtune=native" COMPILER_SUPPORTS_MTUNE_NATIVE)
#if(COMPILER_SUPPORTS_MTUNE_NATIVE)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=native")
#endif()

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

add_subdirectory(lib)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TBBGet.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TBBBuild.cmake)
#tbb_get(TBB_ROOT tbb_root SOURCE_CODE)
#tbb_build(TBB_ROOT ${tbb_root} CONFIG_DIR TBB_DIR)
#find_package(TBB REQUIRED tbb)

add_definitions(-DPERFORMANCE_MONITORING -DTBB_USE_GLIBCXX_VERSION=70400)

#add_definitions(-fprofile-generate)
#set(LINKER_FLAGS "${LINKER_FLAGS} -fprofile-generate")

#add_definitions(-fprofile-use)
#set(LINKER_FLAGS "${LINKER_FLAGS} -fprofile-use")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")

add_library(enumeration_tool INTERFACE)
target_include_directories(enumeration_tool INTERFACE include)
target_link_libraries(enumeration_tool INTERFACE
        nauty kitty fmt iterator_tpl magic_enum json range-v3 robin_hood
#        ${TBB_IMPORTED_TARGETS}
        )
target_compile_definitions(enumeration_tool INTERFACE)

#add_subdirectory(examples)
add_subdirectory(test)
add_subdirectory(experiments)