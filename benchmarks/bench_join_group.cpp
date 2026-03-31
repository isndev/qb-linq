/**
 * @file bench_join_group.cpp
 * @brief Join and group_join materialization vs hand-written nested loops / grouping.
 */

#include <benchmark/benchmark.h>

#include <unordered_map>
#include <vector>

#include "detail/bench_common.hpp"
#include "qb/linq.h"

namespace {

struct L {
    int k;
    int v;
};
struct R {
    int k;
    int w;
};

void build_outer(std::vector<L>& out, std::size_t n)
{
    out.clear();
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i)
        out.push_back(L{static_cast<int>(i % 97), static_cast<int>(i)});
}

void build_inner(std::vector<R>& out, std::size_t n)
{
    out.clear();
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i)
        out.push_back(R{static_cast<int>((i * 3) % 97), static_cast<int>(i + 1)});
}

} // namespace

static void Naive_JoinSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<L> left;
    std::vector<R> right;
    build_outer(left, n);
    build_inner(right, n);
    for (auto _ : state) {
        long long s = 0;
        for (L const& a : left) {
            for (R const& b : right) {
                if (a.k == b.k)
                    s += static_cast<long long>(a.v) + b.w;
            }
        }
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n)
        * static_cast<int64_t>(n));
}
BENCHMARK(Naive_JoinSum)->Arg(256)->Arg(512);

static void Linq_JoinSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<L> left;
    std::vector<R> right;
    build_outer(left, n);
    build_inner(right, n);
    for (auto _ : state) {
        long long s = 0;
        auto j = qb::linq::from(left).join(right,
            [](L const& x) { return x.k; },
            [](R const& x) { return x.k; },
            [](L const& a, R const& b) { return a.v + b.w; });
        for (int x : j)
            s += x;
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n)
        * static_cast<int64_t>(n));
}
BENCHMARK(Linq_JoinSum)->Arg(256)->Arg(512);

static void Naive_GroupByBucketSizes(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> keys(n);
    for (std::size_t i = 0; i < n; ++i)
        keys[i] = static_cast<int>(i % 64);
    for (auto _ : state) {
        std::unordered_map<int, std::size_t> cnt;
        for (int k : keys)
            ++cnt[k];
        benchmark::DoNotOptimize(&cnt);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_GroupByBucketSizes)->Arg(10'000)->Arg(100'000);

static void Linq_GroupByBucketSizes(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> keys(n);
    for (std::size_t i = 0; i < n; ++i)
        keys[i] = static_cast<int>(i % 64);
    for (auto _ : state) {
        std::size_t sum = 0;
        auto g = qb::linq::from(keys).group_by([](int x) { return x; });
        for (auto const& pr : g)
            sum += qb::linq::from(pr.second).long_count();
        benchmark::DoNotOptimize(&sum);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_GroupByBucketSizes)->Arg(10'000)->Arg(100'000);

static void Naive_GroupJoinInnerSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<L> left;
    std::vector<R> right;
    build_outer(left, n);
    build_inner(right, n);
    for (auto _ : state) {
        long long s = 0;
        for (L const& a : left) {
            for (R const& b : right) {
                if (a.k == b.k)
                    s += static_cast<long long>(b.w);
            }
        }
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n)
        * static_cast<int64_t>(n));
}
BENCHMARK(Naive_GroupJoinInnerSum)->Arg(256)->Arg(512);

static void Linq_GroupJoinInnerSum(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<L> left;
    std::vector<R> right;
    build_outer(left, n);
    build_inner(right, n);
    for (auto _ : state) {
        long long s = 0;
        auto gj = qb::linq::from(left).group_join(right,
            [](L const& x) { return x.k; },
            [](R const& x) { return x.k; });
        for (auto const& row : gj) {
            for (R const& b : row.second)
                s += static_cast<long long>(b.w);
        }
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n)
        * static_cast<int64_t>(n));
}
BENCHMARK(Linq_GroupJoinInnerSum)->Arg(256)->Arg(512);
