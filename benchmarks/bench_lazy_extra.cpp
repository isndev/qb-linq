/**
 * @file bench_lazy_extra.cpp
 * @brief reverse, concat, stride, scan, skip_last — lazy layers vs explicit loops.
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <vector>

#include "detail/bench_common.hpp"
#include "qb/linq.h"

static void Naive_ReverseSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    std::iota(data.begin(), data.end(), 0);
    for (auto _ : state) {
        long long s = 0;
        for (std::size_t i = 0; i < n; ++i)
            s += data[n - 1 - i];
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_ReverseSum)->Arg(10'000)->Arg(100'000);

static void Linq_ReverseSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    std::iota(data.begin(), data.end(), 0);
    for (auto _ : state) {
        int const s = qb::linq::from(data).reverse().sum();
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_ReverseSum)->Arg(10'000)->Arg(100'000);

static void Naive_ConcatSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> a(n / 2);
    std::vector<int> b(n - n / 2);
    qb_linq_bench::fill_random(a, 7u);
    qb_linq_bench::fill_random(b, 8u);
    for (auto _ : state) {
        long long s = 0;
        for (int x : a)
            s += x;
        for (int x : b)
            s += x;
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_ConcatSum)->Arg(50'000)->Arg(200'000);

static void Linq_ConcatSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> a(n / 2);
    std::vector<int> b(n - n / 2);
    qb_linq_bench::fill_random(a, 7u);
    qb_linq_bench::fill_random(b, 8u);
    // Const snapshots once per fixture — not per iteration (copies would dwarf concat/sum cost).
    std::vector<int> const ca(a.begin(), a.end());
    std::vector<int> const cb(b.begin(), b.end());
    for (auto _ : state) {
        int const s = qb::linq::from(ca).concat(cb).sum();
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_ConcatSum)->Arg(50'000)->Arg(200'000);

static void Naive_Stride2Sum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 9u);
    for (auto _ : state) {
        long long s = 0;
        for (std::size_t i = 0; i < n; i += 2)
            s += data[i];
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>((n + 1) / 2));
}
BENCHMARK(Naive_Stride2Sum)->Arg(50'000)->Arg(200'000);

static void Linq_Stride2Sum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 9u);
    for (auto _ : state) {
        int const s = qb::linq::from(data).stride(2).sum();
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>((n + 1) / 2));
}
BENCHMARK(Linq_Stride2Sum)->Arg(50'000)->Arg(200'000);

static void Naive_ScanLast(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 13u);
    for (auto _ : state) {
        int acc = 0;
        int last = 0;
        for (int x : data) {
            acc += x;
            last = acc;
        }
        benchmark::DoNotOptimize(&last);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_ScanLast)->Arg(20'000)->Arg(100'000);

static void Linq_ScanLast(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 13u);
    for (auto _ : state) {
        int last = 0;
        for (int x : qb::linq::from(data).scan(0, [](int a, int v) { return a + v; }))
            last = x;
        benchmark::DoNotOptimize(&last);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_ScanLast)->Arg(20'000)->Arg(100'000);

static void Naive_SkipLastSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::size_t const k = (std::min)(n / 5, static_cast<std::size_t>(5'000));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 17u);
    std::size_t const counted = n > k ? n - k : 0;
    for (auto _ : state) {
        long long s = 0;
        for (std::size_t i = 0; i < counted; ++i)
            s += static_cast<long long>(data[i]);
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(counted));
}
BENCHMARK(Naive_SkipLastSum)->Arg(20'000)->Arg(120'000);

static void Linq_SkipLastSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::size_t const k = (std::min)(n / 5, static_cast<std::size_t>(5'000));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 17u);
    std::size_t const counted = n > k ? n - k : 0;
    for (auto _ : state) {
        long long s = qb::linq::from(data).skip_last(k).sum();
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(counted));
}
BENCHMARK(Linq_SkipLastSum)->Arg(20'000)->Arg(120'000);

/// Skip until first \c x % 7 == 0, then sum the tail (meaningful on 0..999 random data).
static void Naive_SkipUntilDiv7Sum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 29u);
    for (auto _ : state) {
        long long s = 0;
        std::size_t i = 0;
        while (i < n && (data[i] % 7) != 0)
            ++i;
        for (; i < n; ++i)
            s += data[i];
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_SkipUntilDiv7Sum)->Arg(50'000)->Arg(200'000);

static void Linq_SkipUntilDiv7Sum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 29u);
    for (auto _ : state) {
        long long s = qb::linq::from(data).skip_while(+[](int x) { return (x % 7) != 0; }).sum();
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_SkipUntilDiv7Sum)->Arg(50'000)->Arg(200'000);
