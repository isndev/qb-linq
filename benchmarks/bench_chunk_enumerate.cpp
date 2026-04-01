/**
 * @file bench_chunk_enumerate.cpp
 * @brief chunk, enumerate, default_if_empty, distinct — vs manual loops.
 */

#include <benchmark/benchmark.h>

#include <vector>

#include "detail/bench_common.hpp"
#include "qb/linq.h"

static void Naive_Chunk2FlattenSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 19u);
    for (auto _ : state) {
        long long s = 0;
        for (std::size_t i = 0; i + 1 < n; i += 2) {
            s += data[i];
            s += data[i + 1];
        }
        if (n % 2 == 1)
            s += data[n - 1];
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_Chunk2FlattenSum)->Arg(10'000)->Arg(80'000);

static void Linq_Chunk2Sum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 19u);
    for (auto _ : state) {
        long long s = 0;
        for (auto const& chunk : qb::linq::from(data).chunk(2)) {
            for (int x : chunk)
                s += x;
        }
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_Chunk2Sum)->Arg(10'000)->Arg(80'000);

static void Naive_EnumerateIndexSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 23u);
    for (auto _ : state) {
        long long s = 0;
        for (std::size_t i = 0; i < n; ++i)
            s += static_cast<long long>(i) ^ static_cast<unsigned>(data[i]);
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_EnumerateIndexSum)->Arg(20'000)->Arg(100'000);

static void Linq_EnumerateIndexSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 23u);
    for (auto _ : state) {
        long long s = 0;
        for (auto const& pr : qb::linq::from(data).enumerate())
            s += static_cast<long long>(pr.first) ^ static_cast<unsigned>(pr.second);
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_EnumerateIndexSum)->Arg(20'000)->Arg(100'000);

static void Linq_DefaultIfEmptySum(benchmark::State& state)
{
    std::vector<int> const empty;
    for (auto _ : state) {
        int const s = qb::linq::from(empty).default_if_empty(0).sum();
        benchmark::DoNotOptimize(&s);
    }
}
BENCHMARK(Linq_DefaultIfEmptySum);

static void Naive_DistinctCount(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 29u);
    for (int& x : data)
        x = std::abs(x) % 64;
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> w = data;
        state.ResumeTiming();
        std::vector<int> u;
        u.reserve(w.size());
        for (int x : w) {
            bool seen = false;
            for (int y : u) {
                if (y == x) {
                    seen = true;
                    break;
                }
            }
            if (!seen)
                u.push_back(x);
        }
        benchmark::DoNotOptimize(u.size());
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_DistinctCount)->Arg(2'000)->Arg(15'000);

static void Linq_DistinctCount(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 29u);
    for (int& x : data)
        x = std::abs(x) % 64;
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> w = data;
        state.ResumeTiming();
        std::size_t const c = qb::linq::from(w).distinct().long_count();
        benchmark::DoNotOptimize(&c);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_DistinctCount)->Arg(2'000)->Arg(15'000);
