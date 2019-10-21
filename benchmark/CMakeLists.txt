add_executable(run_benchmarks
        benchmark.cpp
        ltl_benchmark.cpp
        increase_stack_benchmark.cpp
        division_benchmark.cpp
        accumulation_benchmark.cpp
        smt_benchmark.cpp
        ltl_grammar_benchmark.cpp
        )
target_link_libraries(run_benchmarks ${benchmark_LIBRARIES} enumeration_tool copycat z3_expr)
target_include_directories(run_benchmarks PUBLIC ${CMAKE_SOURCE_DIR}/test/catch2)