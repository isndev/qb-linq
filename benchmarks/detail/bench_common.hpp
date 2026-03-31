#pragma once

/// @file bench_common.hpp
/// @brief Shared helpers for Google Benchmark translation units (no ODR-one definitions).

#include <cstdint>
#include <random>
#include <vector>

namespace qb_linq_bench {

inline void fill_random(std::vector<int>& out, unsigned seed)
{
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist(0, 999);
    for (int& x : out)
        x = dist(gen);
}

} // namespace qb_linq_bench
