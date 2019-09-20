cmake_minimum_required(VERSION 3.10)
project(z3_expr VERSION 4.8.6.0)

set(CMAKE_CXX_STANDARD 17)

################################################################################
# Project version
################################################################################
set(Z3_FULL_VERSION_STR "${z3_expr_VERSION}") # Note this might be modified
message(STATUS "Z3 version ${z3_expr_VERSION}")
set(Z3_VERSION_MAJOR "4")
set(Z3_VERSION_MINOR "8")
set(Z3_VERSION_PATCH "6")
set(Z3_VERSION_TWEAK "0")

################################################################################
# Useful CMake functions/Macros
################################################################################
include(CheckCXXSourceCompiles)

################################################################################
# Compiler flags for Z3 components.
# Subsequent commands will append to this
################################################################################
set(Z3_COMPONENT_CXX_DEFINES "")
set(Z3_COMPONENT_CXX_FLAGS "")
set(Z3_COMPONENT_EXTRA_INCLUDE_DIRS "")
set(Z3_DEPENDENT_LIBS "")
set(Z3_DEPENDENT_EXTRA_CXX_LINK_FLAGS "")

################################################################################
# Find Python
################################################################################
find_package(PythonInterp REQUIRED)
message(STATUS "PYTHON_EXECUTABLE: ${PYTHON_EXECUTABLE}")

################################################################################
# Target architecture detection
################################################################################
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/target_arch_detect.cmake)
detect_target_architecture(TARGET_ARCHITECTURE)
message(STATUS "Detected target architecture: ${TARGET_ARCHITECTURE}")

################################################################################
# Function for detecting C++ compiler flag support
################################################################################
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/z3_add_cxx_flag.cmake)

################################################################################
# Threading support
################################################################################
find_package(Threads)
list(APPEND Z3_DEPENDENT_LIBS ${CMAKE_THREAD_LIBS_INIT})

################################################################################
# Compiler warnings
################################################################################
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/compiler_warnings.cmake)

################################################################################
# Link time optimization
################################################################################
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/compiler_lto.cmake)

################################################################################
# CMake build file locations
################################################################################
# To mimic the python build system output these into the root of the build
# directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

################################################################################
# Extra dependencies for build rules that use the Python infrastructure to
# generate files used for Z3's build. Changes to these files will trigger
# a rebuild of all the generated files.
################################################################################
# Note: ``update_api.py`` is deliberately not here because it not used
# to generate every generated file. The targets that need it list it explicitly.
set(Z3_GENERATED_FILE_EXTRA_DEPENDENCIES
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/mk_genfile_common.py"
        )

add_definitions("-D_MP_INTERNAL")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/api/c++)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/api)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/src/)

################################################################################
# Z3 components, library and executables
################################################################################
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/z3_add_component.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/z3_append_linker_flag_list_to_target.cmake)

add_subdirectory(src)

get_property(Z3_LIBZ3_COMPONENTS_LIST GLOBAL PROPERTY Z3_LIBZ3_COMPONENTS)
set (object_files "")
foreach (component ${Z3_LIBZ3_COMPONENTS_LIST})
    list(APPEND object_files $<TARGET_OBJECTS:${component}>)
endforeach()

########################################

add_library(z3_expr ${object_files})
target_include_directories(z3_expr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/api)
target_link_libraries(z3_expr PRIVATE ${Z3_DEPENDENT_LIBS})

#add_executable(test main.cpp)
#target_link_libraries(test z3_expr)