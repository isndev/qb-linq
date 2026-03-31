/// @file bench_zip_tail.cpp
/// @brief Zip dot-style accumulation and tail slicing vs naive loops (hot compositional paths).

#include <algorithm>
#include <cstdint>
#include <vector>

#include <benchmark/benchmark.h>

#include "detail/bench_common.hpp"
#include "qb/linq.h"

namespace {

void Naive_ZipProductSum(benchmark::State& state)
{
    int const n = static_cast<int>(state.range(0));
    std::vector<int> a(static_cast<std::size_t>(n));
    std::vector<int> b(static_cast<std::size_t>(n));
    qb_linq_bench::fill_random(a, 1u);
    qb_linq_bench::fill_random(b, 2u);
    for (auto _ : state) {
        std::int64_t s = 0;
        for (int i = 0; i < n; ++i)
            s += static_cast<std::int64_t>(a[static_cast<std::size_t>(i)])
                * static_cast<std::int64_t>(b[static_cast<std::size_t>(i)]);
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * n);
}

void Linq_ZipProductSum(benchmark::State& state)
{
    int const n = static_cast<int>(state.range(0));
    std::vector<int> a(static_cast<std::size_t>(n));
    std::vector<int> b(static_cast<std::size_t>(n));
    qb_linq_bench::fill_random(a, 1u);
    qb_linq_bench::fill_random(b, 2u);
    for (auto _ : state) {
        std::int64_t s = qb::linq::from(a)
                           .zip(b)
                           .select([](auto const& pr) {
                               return static_cast<std::int64_t>(pr.first)
                                   * static_cast<std::int64_t>(pr.second);
                           })
                           .sum();
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * n);
}

void Linq_ZipFoldProductSum(benchmark::State& state)
{
    int const n = static_cast<int>(state.range(0));
    std::vector<int> a(static_cast<std::size_t>(n));
    std::vector<int> b(static_cast<std::size_t>(n));
    qb_linq_bench::fill_random(a, 1u);
    qb_linq_bench::fill_random(b, 2u);
    for (auto _ : state) {
        std::int64_t s = qb::linq::from(a).zip_fold(
            b,
            std::int64_t{0},
            [](std::int64_t acc, int x, int y) {
                return acc + static_cast<std::int64_t>(x) * static_cast<std::int64_t>(y);
            });
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * n);
}

void Naive_TailSum(benchmark::State& state)
{
    int const n = static_cast<int>(state.range(0));
    int const tail = std::min(n / 4, n > 0 ? n : 0);
    std::vector<int> v(static_cast<std::size_t>(n));
    qb_linq_bench::fill_random(v, 3u);
    for (auto _ : state) {
        std::int64_t s = 0;
        int const start = n - tail;
        for (int i = start; i < n; ++i)
            s += v[static_cast<std::size_t>(i)];
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * std::max(tail, 0));
}

void Linq_TakeLastSum(benchmark::State& state)
{
    int const n = static_cast<int>(state.range(0));
    int const tail = std::min(n / 4, n > 0 ? n : 0);
    std::vector<int> v(static_cast<std::size_t>(n));
    qb_linq_bench::fill_random(v, 3u);
    for (auto _ : state) {
        std::int64_t s = qb::linq::from(v).take_last(static_cast<std::size_t>(tail)).sum();
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * std::max(tail, 0));
}

} // namespace

BENCHMARK(Naive_ZipProductSum)->Arg(10'000)->Arg(80'000);
BENCHMARK(Linq_ZipProductSum)->Arg(10'000)->Arg(80'000);
BENCHMARK(Linq_ZipFoldProductSum)->Arg(10'000)->Arg(80'000);
BENCHMARK(Naive_TailSum)->Arg(20'000)->Arg(150'000);
BENCHMARK(Linq_TakeLastSum)->Arg(20'000)->Arg(150'000);

void Naive_SkipTakeWindowSum(benchmark::State& state)
{
    int const n = static_cast<int>(state.range(0));
    int const skip = n / 4;
    int const take = (std::min)(8'000, (std::max)(0, n - skip));
    std::vector<int> v(static_cast<std::size_t>(n));
    qb_linq_bench::fill_random(v, 4u);
    for (auto _ : state) {
        std::int64_t s = 0;
        int const end = (std::min)(n, skip + take);
        for (int i = skip; i < end; ++i)
            s += v[static_cast<std::size_t>(i)];
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * take);
}

void Linq_SkipTakeWindowSum(benchmark::State& state)
{
    int const n = static_cast<int>(state.range(0));
    int const skip = n / 4;
    int const take = (std::min)(8'000, (std::max)(0, n - skip));
    std::vector<int> v(static_cast<std::size_t>(n));
    qb_linq_bench::fill_random(v, 4u);
    for (auto _ : state) {
        std::int64_t s = qb::linq::from(v).skip(static_cast<std::size_t>(skip)).take(take).sum();
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * take);
}

BENCHMARK(Naive_SkipTakeWindowSum)->Arg(50'000)->Arg(200'000);
BENCHMARK(Linq_SkipTakeWindowSum)->Arg(50'000)->Arg(200'000);
