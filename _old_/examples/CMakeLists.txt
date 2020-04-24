add_executable(ctl_enumeration ctl.cpp)
target_link_libraries(ctl_enumeration enumeration_tool ctl_formula)

add_executable(ltl_enumeration ltl.cpp)
target_link_libraries(ltl_enumeration enumeration_tool copycat)

add_executable(xmg_enumeration xmg.cpp)
target_link_libraries(xmg_enumeration enumeration_tool mockturtle)

add_executable(smt_enumeration smt.cpp)
target_link_libraries(smt_enumeration enumeration_tool libz3)

add_executable(partial_dag_test partial_dag.cpp)
target_link_libraries(partial_dag_test enumeration_tool kitty nauty fmt copycat)

add_executable(partial_enumeration partial_enumeration.cpp)
target_link_libraries(partial_enumeration enumeration_tool kitty nauty fmt copycat)

add_executable(aig aig.cpp)
target_link_libraries(aig enumeration_tool mockturtle)

add_executable(maj5 maj5.cpp)
target_link_libraries(maj5 enumeration_tool mockturtle)

add_executable(maj5_1 maj5_1.cpp)
target_link_libraries(maj5_1 enumeration_tool mockturtle)

add_executable(maj5_test maj5_test.cpp)
target_link_libraries(maj5_test enumeration_tool mockturtle)

add_executable(maj7 maj7.cpp)
target_link_libraries(maj7 enumeration_tool mockturtle)

add_executable(npn4 npn4.cpp)
target_link_libraries(npn4 enumeration_tool mockturtle cxxopts json fmt)

add_executable(npn4_test npn4_test.cpp)
target_link_libraries(npn4_test enumeration_tool mockturtle)

add_executable(npn4_1 npn4_1.cpp)
target_link_libraries(npn4_1 enumeration_tool mockturtle)

add_executable(experiments experiments.cpp)
target_link_libraries(experiments enumeration_tool mockturtle)