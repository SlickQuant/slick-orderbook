# Google Benchmark
include(FetchContent)
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)

# Disable benchmark tests
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(benchmark)

# Common benchmark settings
add_compile_options(
    $<$<CONFIG:Release>:-DNDEBUG>
)

# All benchmarks need access to private headers for compiled mode
set(BENCHMARK_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/include_private
)

# Benchmark executables
add_executable(bench_orderbook_l2 bench_orderbook_l2.cpp)
target_link_libraries(bench_orderbook_l2 PRIVATE slick::orderbook benchmark::benchmark)
target_include_directories(bench_orderbook_l2 PRIVATE ${BENCHMARK_INCLUDE_DIRS})

add_executable(bench_orderbook_l3 bench_orderbook_l3.cpp)
target_link_libraries(bench_orderbook_l3 PRIVATE slick::orderbook benchmark::benchmark)
target_include_directories(bench_orderbook_l3 PRIVATE ${BENCHMARK_INCLUDE_DIRS})

add_executable(bench_orderbook_manager bench_orderbook_manager.cpp)
target_link_libraries(bench_orderbook_manager PRIVATE slick::orderbook benchmark::benchmark)
target_include_directories(bench_orderbook_manager PRIVATE ${BENCHMARK_INCLUDE_DIRS})

add_executable(bench_observer_overhead bench_observer_overhead.cpp)
target_link_libraries(bench_observer_overhead PRIVATE slick::orderbook benchmark::benchmark)
target_include_directories(bench_observer_overhead PRIVATE ${BENCHMARK_INCLUDE_DIRS})

add_executable(bench_memory_usage bench_memory_usage.cpp)
target_link_libraries(bench_memory_usage PRIVATE slick::orderbook benchmark::benchmark)
target_include_directories(bench_memory_usage PRIVATE ${BENCHMARK_INCLUDE_DIRS})

add_executable(bench_market_replay bench_market_replay.cpp)
target_link_libraries(bench_market_replay PRIVATE slick::orderbook benchmark::benchmark)
target_include_directories(bench_market_replay PRIVATE ${BENCHMARK_INCLUDE_DIRS})

add_executable(bench_cache_alignment bench_cache_alignment.cpp)
target_link_libraries(bench_cache_alignment PRIVATE slick::orderbook benchmark::benchmark)
# Allow access to private headers for alignment verification
target_include_directories(bench_cache_alignment PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include_private
)

# Custom benchmark runner for all benchmarks
add_custom_target(run_benchmarks
    COMMAND bench_orderbook_l2 --benchmark_out=bench_l2.json --benchmark_out_format=json
    COMMAND bench_orderbook_l3 --benchmark_out=bench_l3.json --benchmark_out_format=json
    COMMAND bench_orderbook_manager --benchmark_out=bench_manager.json --benchmark_out_format=json
    COMMAND bench_observer_overhead --benchmark_out=bench_observer.json --benchmark_out_format=json
    COMMAND bench_memory_usage --benchmark_out=bench_memory.json --benchmark_out_format=json
    COMMAND bench_market_replay --benchmark_out=bench_market_replay.json --benchmark_out_format=json
    COMMAND bench_cache_alignment --benchmark_out=bench_cache_alignment.json --benchmark_out_format=json
    DEPENDS bench_orderbook_l2 bench_orderbook_l3 bench_orderbook_manager
            bench_observer_overhead bench_memory_usage bench_market_replay bench_cache_alignment
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running all benchmarks..."
)
