/**
 * @file bench_set_ops.cpp
 * @brief except / intersect / union_with / distinct materialization vs STL-heavy references.
 */

#include <benchmark/benchmark.h>

#include <unordered_set>
#include <vector>

#include "detail/bench_common.hpp"
#include "qb/linq.h"

static void Naive_Except(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> main(n);
    std::vector<int> ban(n / 4);
    qb_linq_bench::fill_random(main, 1u);
    qb_linq_bench::fill_random(ban, 2u);
    for (int& x : ban)
        x = std::abs(x) % 256;
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> m = main;
        std::vector<int> b = ban;
        state.ResumeTiming();
        std::unordered_set<int> s(b.begin(), b.end());
        std::vector<int> out;
        out.reserve(m.size());
        for (int x : m) {
            if (s.find(x) == s.end())
                out.push_back(x);
        }
        benchmark::DoNotOptimize(out.data());
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_Except)->Arg(20'000)->Arg(100'000);

static void Linq_Except(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> main(n);
    std::vector<int> ban(n / 4);
    qb_linq_bench::fill_random(main, 1u);
    qb_linq_bench::fill_random(ban, 2u);
    for (int& x : ban)
        x = std::abs(x) % 256;
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> m = main;
        std::vector<int> b = ban;
        state.ResumeTiming();
        auto out = qb::linq::from(m).except(b).to_vector();
        benchmark::DoNotOptimize(out.begin());
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_Except)->Arg(20'000)->Arg(100'000);

static void Naive_Intersect(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> a(n);
    std::vector<int> b(n);
    qb_linq_bench::fill_random(a, 3u);
    qb_linq_bench::fill_random(b, 4u);
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> va = a;
        std::vector<int> vb = b;
        state.ResumeTiming();
        std::unordered_set<int> want(vb.begin(), vb.end());
        std::vector<int> out;
        for (int x : va) {
            if (want.find(x) != want.end())
                out.push_back(x);
        }
        benchmark::DoNotOptimize(out.data());
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_Intersect)->Arg(20'000)->Arg(100'000);

static void Linq_Intersect(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> a(n);
    std::vector<int> b(n);
    qb_linq_bench::fill_random(a, 3u);
    qb_linq_bench::fill_random(b, 4u);
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> va = a;
        std::vector<int> vb = b;
        state.ResumeTiming();
        auto out = qb::linq::from(va).intersect(vb).to_vector();
        benchmark::DoNotOptimize(out.begin());
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_Intersect)->Arg(20'000)->Arg(100'000);

static void Naive_UnionDistinct(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> a(n / 2);
    std::vector<int> b(n / 2);
    qb_linq_bench::fill_random(a, 5u);
    qb_linq_bench::fill_random(b, 6u);
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> va = a;
        std::vector<int> vb = b;
        state.ResumeTiming();
        std::unordered_set<int> u;
        for (int x : va)
            u.insert(x);
        for (int x : vb)
            u.insert(x);
        benchmark::DoNotOptimize(&u);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_UnionDistinct)->Arg(20'000)->Arg(100'000);

static void Linq_UnionWith(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> a(n / 2);
    std::vector<int> b(n / 2);
    qb_linq_bench::fill_random(a, 5u);
    qb_linq_bench::fill_random(b, 6u);
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> const va = a;
        std::vector<int> const vb = b;
        state.ResumeTiming();
        auto out = qb::linq::from(va).union_with(vb).to_vector();
        benchmark::DoNotOptimize(out.begin());
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_UnionWith)->Arg(20'000)->Arg(100'000);
