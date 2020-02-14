add_library(copycat INTERFACE)
target_include_directories(copycat INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/copycat)

add_library(ctl_formula INTERFACE)
target_include_directories(ctl_formula INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ctl_formula)

add_subdirectory(nauty)

add_subdirectory(fmt-6.0.0)

add_subdirectory(mockturtle)

if (NOT TARGET kitty)
  add_library(kitty INTERFACE) # v0.4
  target_include_directories(kitty INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/kitty)
endif()

if (NOT TARGET iterator_tpl)
  add_library(iterator_tpl INTERFACE) # v0.4
  target_include_directories(iterator_tpl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Simple-Iterator-Template)
endif()

add_subdirectory(z3 EXCLUDE_FROM_ALL)

add_subdirectory(cxxopts-2.2.0)

if (NOT TARGET magic_enum)
  add_library(magic_enum INTERFACE) # v0.6.4
  target_include_directories(magic_enum INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/magic_enum)
endif()

if (NOT TARGET json)
  add_library(json INTERFACE) # v3.7.3
  target_include_directories(json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/nlohmann)
endif()

if (NOT TARGET CLI11)
  add_library(CLI11 INTERFACE) # v1.9.0
  target_include_directories(json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/CLI11)
endif()