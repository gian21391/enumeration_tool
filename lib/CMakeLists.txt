add_library(copycat INTERFACE)
target_include_directories(copycat INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/copycat)

add_library(ctl_formula INTERFACE)
target_include_directories(ctl_formula INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ctl_formula)

add_subdirectory(mockturtle)

add_subdirectory(z3_expr)