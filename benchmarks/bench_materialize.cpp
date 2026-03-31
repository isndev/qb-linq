/**
 * @file bench_materialize.cpp
 * @brief Materializing paths: sort / copy vs qb::linq order_by and to_vector.
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <numeric>
#include <vector>

#include "detail/bench_common.hpp"
#include "qb/linq.h"

static void Naive_SortCopy(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 99u);
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> work = data;
        state.ResumeTiming();
        std::sort(work.begin(), work.end());
        benchmark::DoNotOptimize(work.data());
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_SortCopy)->Arg(1 << 10)->Arg(50'000);

static void Linq_OrderByToVector(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 99u);
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> work = data;
        state.ResumeTiming();
        auto sorted = qb::linq::from(work).order_by(qb::linq::asc([](int x) { return x; })).to_vector();
        benchmark::DoNotOptimize(sorted.begin());
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_OrderByToVector)->Arg(1 << 10)->Arg(50'000);

static void Naive_CopyToVector(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    std::iota(data.begin(), data.end(), 0);
    for (auto _ : state) {
        std::vector<int> copy;
        copy.reserve(n);
        for (int x : data)
            copy.push_back(x);
        benchmark::DoNotOptimize(copy.data());
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_CopyToVector)->Arg(50'000)->Arg(200'000);

static void Linq_ToVector(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    std::iota(data.begin(), data.end(), 0);
    for (auto _ : state) {
        auto copy = qb::linq::from(data).to_vector();
        benchmark::DoNotOptimize(copy.begin());
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_ToVector)->Arg(50'000)->Arg(200'000);

static void Linq_DistinctMaterialize(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 3u);
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> work = data;
        state.ResumeTiming();
        auto u = qb::linq::from(work).distinct().to_vector();
        benchmark::DoNotOptimize(u.begin());
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_DistinctMaterialize)->Arg(10'000)->Arg(100'000);
