# GoogleTest via FetchContent (reproducible; override with CMAKE_PREFIX_PATH + find_package if desired).
include(FetchContent)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(googletest)

set(_qb_linq_test_sources
    linq_version_test.cpp
    linq_pipeline_test.cpp
    linq_terminal_test.cpp
    linq_terminals_extended_test.cpp
    linq_statistics_test.cpp
    linq_statistics_by_key_test.cpp
    linq_materialize_test.cpp
    linq_materialized_index_test.cpp
    linq_slice_tail_test.cpp
    linq_factories_test.cpp
    linq_fused_aggregates_test.cpp
    linq_join_set_test.cpp
    linq_relational_extended_test.cpp
    linq_order_group_lookup_test.cpp
    linq_ordering_advanced_test.cpp
    linq_views_compositional_test.cpp
    linq_select_each_test.cpp
    linq_polymorphic_test.cpp
    linq_boundaries_empty_test.cpp
    linq_search_comparison_test.cpp
    linq_zip_take_where_test.cpp
    linq_predicates_robustness_test.cpp
    linq_scan_and_iterator_layout_test.cpp
    linq_terminal_minmax_stats_test.cpp
    linq_surfaces_test.cpp
    linq_chaining_linq_style_test.cpp
    linq_exceptions_test.cpp
    linq_property_fuzz_test.cpp
    linq_improvements_test.cpp
)

add_executable(qb_linq_tests ${_qb_linq_test_sources})
target_link_libraries(qb_linq_tests PRIVATE qb::linq GTest::gtest_main)
target_compile_features(qb_linq_tests PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(qb_linq_tests PRIVATE /W4 /permissive-)
else()
    target_compile_options(qb_linq_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(GoogleTest)
gtest_discover_tests(qb_linq_tests
    DISCOVERY_TIMEOUT 60
)

# single_header/linq.h — one TU, no qb::linq include path (must stay in sync via qb_linq_single_header)
add_executable(qb_linq_single_header_test linq_single_header_test.cpp)
target_include_directories(qb_linq_single_header_test PRIVATE "${CMAKE_SOURCE_DIR}/single_header")
target_link_libraries(qb_linq_single_header_test PRIVATE GTest::gtest_main)
target_compile_features(qb_linq_single_header_test PRIVATE cxx_std_17)
if(MSVC)
    target_compile_options(qb_linq_single_header_test PRIVATE /W4 /permissive- /bigobj)
else()
    target_compile_options(qb_linq_single_header_test PRIVATE -Wall -Wextra -Wpedantic)
endif()
gtest_discover_tests(qb_linq_single_header_test
    DISCOVERY_TIMEOUT 60
)
