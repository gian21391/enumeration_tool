add_executable(ctl_enumeration ctl.cpp)
target_link_libraries(ctl_enumeration enumeration_tool ctl_formula)

add_executable(ltl_enumeration ltl.cpp)
target_link_libraries(ltl_enumeration enumeration_tool copycat)

add_executable(xmg_enumeration xmg.cpp)
target_link_libraries(xmg_enumeration enumeration_tool mockturtle)

add_executable(smt_enumeration smt.cpp)
target_link_libraries(smt_enumeration enumeration_tool z3_expr)

add_executable(partial_dag_test partial_dag.cpp)
target_link_libraries(partial_dag_test enumeration_tool kitty nauty fmt copycat)

add_executable(partial_enumeration partial_enumeration.cpp)
target_link_libraries(partial_enumeration enumeration_tool kitty nauty fmt copycat)

add_executable(aig aig.cpp)
target_link_libraries(aig enumeration_tool mockturtle)

add_executable(maj5 maj5.cpp)
target_link_libraries(maj5 enumeration_tool mockturtle)

add_executable(experiments experiments.cpp)
target_link_libraries(experiments enumeration_tool mockturtle)