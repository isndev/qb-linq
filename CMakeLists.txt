cmake_minimum_required(VERSION 3.10)
project(qb-linq)

# qb minimum requirements
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(CMAKE_BUILD_TYPE Release)
option(QB_BUILD_TEST OFF)

add_library(qb-linq INTERFACE)
target_include_directories(qb-linq INTERFACE include)

if (QB_BUILD_TEST)
    add_executable(qb-linq-benchmark benchmark.cpp)
    target_link_libraries(qb-linq-benchmark qb-linq)

    add_executable(qb-linq-overhead overhead.cpp)
    target_link_libraries(qb-linq-overhead qb-linq)
endif ()