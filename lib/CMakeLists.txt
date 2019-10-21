add_library(copycat INTERFACE)
target_include_directories(copycat INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/copycat)

add_library(ctl_formula INTERFACE)
target_include_directories(ctl_formula INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ctl_formula)

add_subdirectory(mockturtle)

add_subdirectory(z3_expr)

if (NOT TARGET kitty)
  add_library(kitty INTERFACE) # v0.4
  target_include_directories(kitty INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/kitty)
endif()

add_subdirectory(nauty)

if (NOT TARGET iterator_tpl)
  add_library(iterator_tpl INTERFACE) # v0.4
  target_include_directories(iterator_tpl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Simple-Iterator-Template)
endif()

#add_subdirectory(fmt-6.0.0)