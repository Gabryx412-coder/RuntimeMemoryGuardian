# ==============================================================================
# Runtime Memory Guardian - Benchmark Suite
# ==============================================================================

include(FetchContent)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.1
)

FetchContent_MakeAvailable(googlebenchmark)

function(rmg_add_benchmark bench_name)
    add_executable(${bench_name} ${ARGN})
    target_link_libraries(${bench_name} PRIVATE RMG::rmg benchmark::benchmark)
    rmg_enable_warnings(${bench_name})
    set_target_properties(${bench_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmarks"
    )
endfunction()

rmg_add_benchmark(bench_memory_scanner  bench_memory_scanner.cpp)
rmg_add_benchmark(bench_hash_provider   bench_hash_provider.cpp)
rmg_add_benchmark(bench_hook_detector   bench_hook_detector.cpp)