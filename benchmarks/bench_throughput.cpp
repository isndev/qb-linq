/**
 * @file bench_throughput.cpp
 * @brief Throughput: naive loops vs qb::linq terminals and simple pipelines.
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <numeric>
#include <vector>

#include "detail/bench_common.hpp"
#include "qb/linq.h"

static void Naive_Sum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 42u);
    for (auto _ : state) {
        long long s = 0;
        for (int x : data)
            s += x;
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_Sum)->Arg(1 << 10)->Arg(50'000)->Arg(200'000);

static void Linq_Sum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 42u);
    for (auto _ : state) {
        int const s = qb::linq::from(data).sum();
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_Sum)->Arg(1 << 10)->Arg(50'000)->Arg(200'000);

static void Naive_SelectSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 42u);
    for (auto _ : state) {
        long long s = 0;
        for (int x : data)
            s += x * 2;
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_SelectSum)->Arg(1 << 10)->Arg(50'000)->Arg(200'000);

static void Linq_SelectSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 42u);
    for (auto _ : state) {
        int const s = qb::linq::from(data).select([](int x) { return x * 2; }).sum();
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_SelectSum)->Arg(1 << 10)->Arg(50'000)->Arg(200'000);

static void Naive_WhereSum(benchmark::State& state)
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
BENCHMARK(Naive_WhereSum)->Arg(1 << 10)->Arg(50'000)->Arg(200'000);

static void Linq_WhereSum(benchmark::State& state)
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
BENCHMARK(Linq_WhereSum)->Arg(1 << 10)->Arg(50'000)->Arg(200'000);

static void Naive_TakeSum(benchmark::State& state)
{
    int const k = 1000;
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 42u);
    for (auto _ : state) {
        long long s = 0;
        int taken = 0;
        for (int x : data) {
            if (taken++ >= k)
                break;
            s += x;
        }
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations())
        * static_cast<int64_t>(std::min(k, static_cast<int>(n))));
}
BENCHMARK(Naive_TakeSum)->Arg(50'000)->Arg(200'000);

static void Linq_TakeSum(benchmark::State& state)
{
    int const k = 1000;
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 42u);
    for (auto _ : state) {
        int const s = qb::linq::from(data).take(k).sum();
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations())
        * static_cast<int64_t>(std::min(k, static_cast<int>(n))));
}
BENCHMARK(Linq_TakeSum)->Arg(50'000)->Arg(200'000);

static void Linq_Chain_SelectWhereSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 42u);
    int const cut = 400;
    for (auto _ : state) {
        int const s = qb::linq::from(data)
                          .select([](int x) { return x / 2; })
                          .where([c = cut](int x) { return x > c; })
                          .sum();
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_Chain_SelectWhereSum)->Arg(50'000)->Arg(200'000);

static void Naive_TakeWhileEvenPrefixSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 23u);
    for (auto _ : state) {
        long long s = 0;
        for (int x : data) {
            if ((x & 1) != 0)
                break;
            s += x;
        }
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_TakeWhileEvenPrefixSum)->Arg(50'000)->Arg(200'000);

static void Linq_TakeWhileEvenPrefixSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 23u);
    for (auto _ : state) {
        long long s = qb::linq::from(data).take_while([](int x) { return (x & 1) == 0; }).sum();
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_TakeWhileEvenPrefixSum)->Arg(50'000)->Arg(200'000);
