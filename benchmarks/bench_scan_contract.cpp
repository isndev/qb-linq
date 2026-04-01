/**
 * @file bench_scan_contract.cpp
 * @brief Scan iteration stress (iterator copies) and select with empty vs stateful functors — regression targets
 *        for `scan_view` (no shared_ptr) and `compressed_fn` EBO.
 */

#include <array>
#include <benchmark/benchmark.h>
#include <cstdint>
#include <vector>

#include "detail/bench_common.hpp"
#include "qb/linq.h"

namespace {

struct EmptySelect {
    int operator()(int x) const { return x * 3; }
};

struct FatSelect {
    std::array<int, 32> scratch{};
    int operator()(int x) const
    {
        return x + static_cast<int>(scratch[0]);
    }
};

} // namespace

/// Walk scan while copying the iterator at every step (stresses iterator copy + shared `F*` path).
static void Linq_ScanSumWithPerStepIteratorCopy(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 41u);
    for (auto _ : state) {
        long long s = 0;
        auto pipe = qb::linq::from(data).scan(0LL, [](long long a, int v) { return a + v; });
        for (auto it = pipe.begin(), e = pipe.end(); it != e;) {
            auto cur = it;
            s += *cur;
            ++it;
        }
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_ScanSumWithPerStepIteratorCopy)->Arg(10'000)->Arg(80'000);

static void Linq_ScanSumStraightWalk(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 41u);
    for (auto _ : state) {
        long long s = 0;
        for (long long x : qb::linq::from(data).scan(0LL, [](long long a, int v) { return a + v; }))
            s += x;
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_ScanSumStraightWalk)->Arg(10'000)->Arg(80'000);

static void Linq_SelectEmptyFunctorSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 43u);
    for (auto _ : state) {
        long long s = 0;
        for (int x : qb::linq::from(data).select(EmptySelect{}))
            s += x;
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_SelectEmptyFunctorSum)->Arg(20'000)->Arg(120'000);

static void Linq_SelectFatFunctorSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 43u);
    FatSelect f{};
    for (auto _ : state) {
        long long s = 0;
        for (int x : qb::linq::from(data).select(f))
            s += x;
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_SelectFatFunctorSum)->Arg(20'000)->Arg(120'000);
