add_subdirectory(benchmark)

set(benchmark_BINARY_ROOT_DIR "${CMAKE_CURRENT_BINARY_DIR}/benchmark")
set(benchmark_SOURCE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/benchmark")

find_package(GoogleBenchmark REQUIRED)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

include_directories(${benchmark_INCLUDE_DIRS})

add_executable(run_benchmarks
        ltl_benchmark.cpp
#        increase_stack_benchmark.cpp
#        division_benchmark.cpp
#        accumulation_benchmark.cpp
#        smt_benchmark.cpp
        ltl_grammar_benchmark.cpp
        )
target_link_libraries(run_benchmarks ${benchmark_LIBRARIES} enumeration_tool copycat z3_expr)
target_include_directories(run_benchmarks PUBLIC ${benchmark_INCLUDE_DIRS})