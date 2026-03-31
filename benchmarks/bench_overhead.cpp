/**
 * @file bench_overhead.cpp
 * @brief Pipeline construction / iterator churn vs hoisted pipelines.
 */

#include <benchmark/benchmark.h>

#include <vector>

#include "detail/bench_common.hpp"
#include "qb/linq.h"

static void Overhead_Where_RebuildEachIteration(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 42u);
    for (auto _ : state) {
        auto q = qb::linq::from(data).where([](int x) { return x > 500; });
        benchmark::DoNotOptimize(q.begin());
    }
}
BENCHMARK(Overhead_Where_RebuildEachIteration)->Arg(10'000)->Arg(200'000);

static void Overhead_Where_HoistedOutsideLoop(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 42u);
    auto const pipe = qb::linq::from(data).where([](int x) { return x > 500; });
    for (auto _ : state) {
        int s = 0;
        for (int x : pipe)
            s += x;
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Overhead_Where_HoistedOutsideLoop)->Arg(10'000)->Arg(200'000);
