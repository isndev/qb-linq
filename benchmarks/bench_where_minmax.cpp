/**
 * @file bench_where_minmax.cpp
 * @brief where + first / min_by / reduce vs naive single-pass patterns.
 */

#include <benchmark/benchmark.h>

#include <cstdlib>
#include <vector>

#include "detail/bench_common.hpp"
#include "qb/linq.h"

static void Naive_FirstMatch(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 31u);
    int const cut = 600;
    for (auto _ : state) {
        int found = -1;
        for (int x : data) {
            if (x > cut) {
                found = x;
                break;
            }
        }
        benchmark::DoNotOptimize(&found);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_FirstMatch)->Arg(50'000)->Arg(200'000);

static void Linq_FirstIf(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 31u);
    int const cut = 600;
    for (auto _ : state) {
        int const found = qb::linq::from(data).first_if([c = cut](int x) { return x > c; });
        benchmark::DoNotOptimize(&found);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_FirstIf)->Arg(50'000)->Arg(200'000);

static void Naive_MinByKey(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 37u);
    for (auto _ : state) {
        int best = data[0];
        int best_key = std::abs(data[0]);
        for (std::size_t i = 1; i < n; ++i) {
            int const k = std::abs(data[i]);
            if (k < best_key) {
                best_key = k;
                best = data[i];
            }
        }
        benchmark::DoNotOptimize(&best);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_MinByKey)->Arg(50'000)->Arg(200'000);

static void Linq_MinBy(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 37u);
    for (auto _ : state) {
        int const best = qb::linq::from(data).min_by([](int x) { return std::abs(x); });
        benchmark::DoNotOptimize(&best);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_MinBy)->Arg(50'000)->Arg(200'000);

static void Naive_AggregateHashMix(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 41u);
    for (auto _ : state) {
        int acc = 0;
        for (int x : data)
            acc = (acc * 1315423911 + x) ^ (acc >> 15);
        benchmark::DoNotOptimize(&acc);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_AggregateHashMix)->Arg(50'000)->Arg(200'000);

static void Linq_AggregateHashMix(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 41u);
    for (auto _ : state) {
        int const acc = qb::linq::from(data).aggregate(0, [](int a, int x) {
            return (a * 1315423911 + x) ^ (a >> 15);
        });
        benchmark::DoNotOptimize(&acc);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_AggregateHashMix)->Arg(50'000)->Arg(200'000);
