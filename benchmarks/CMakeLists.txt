# Google Benchmark — FetchContent for reproducible local runs; disable its install rules.
include(FetchContent)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.1
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(qb_linq_benchmark
    bench_throughput.cpp
    bench_fused.cpp
    bench_materialize.cpp
    bench_overhead.cpp
    bench_fixture_range.cpp
    bench_join_group.cpp
    bench_set_ops.cpp
    bench_statistics.cpp
    bench_lazy_extra.cpp
    bench_chunk_enumerate.cpp
    bench_where_minmax.cpp
    bench_zip_tail.cpp
)
target_include_directories(qb_linq_benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(qb_linq_benchmark PRIVATE qb::linq benchmark::benchmark_main)
target_compile_features(qb_linq_benchmark PRIVATE cxx_std_17)

if(NOT MSVC)
    target_compile_options(qb_linq_benchmark PRIVATE -Wall -Wextra)
endif()
