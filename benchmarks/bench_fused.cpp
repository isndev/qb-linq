/**
 * @file bench_fused.cpp
 * @brief Fused terminals (sum_if / count_if) vs explicit where + aggregate.
 */

#include <benchmark/benchmark.h>

#include <vector>

#include "detail/bench_common.hpp"
#include "qb/linq.h"

static void Naive_FilterSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 42u);
    int const cut = 500;
    for (auto _ : state) {
        long long s = 0;
        for (int x : data) {
            if (x > cut)
                s += x;
        }
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_FilterSum)->Arg(50'000)->Arg(200'000);

static void Linq_SumIf(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 42u);
    int const cut = 500;
    for (auto _ : state) {
        int const s = qb::linq::from(data).sum_if([c = cut](int x) { return x > c; });
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_SumIf)->Arg(50'000)->Arg(200'000);

static void Linq_WhereThenSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 42u);
    int const cut = 500;
    for (auto _ : state) {
        int const s = qb::linq::from(data).where([c = cut](int x) { return x > c; }).sum();
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_WhereThenSum)->Arg(50'000)->Arg(200'000);

static void Naive_FilterCount(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 17u);
    for (auto _ : state) {
        std::size_t c = 0;
        for (int x : data) {
            if (x % 3 == 0)
                ++c;
        }
        benchmark::DoNotOptimize(&c);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_FilterCount)->Arg(50'000)->Arg(200'000);

static void Linq_CountIf(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 17u);
    for (auto _ : state) {
        std::size_t const c = qb::linq::from(data).count_if([](int x) { return x % 3 == 0; });
        benchmark::DoNotOptimize(&c);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_CountIf)->Arg(50'000)->Arg(200'000);
