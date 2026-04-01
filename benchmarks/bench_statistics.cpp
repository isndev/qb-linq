/**
 * @file bench_statistics.cpp
 * @brief Median / percentile / variance vs naive sort+copy implementations.
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "detail/bench_common.hpp"
#include "qb/linq.h"

static double naive_median(std::vector<double> const& v)
{
    auto w = v;
    std::sort(w.begin(), w.end());
    std::size_t const n = w.size();
    if (n == 0)
        return 0;
    if (n % 2 == 1)
        return w[n / 2];
    return (w[n / 2 - 1] + w[n / 2]) / 2.0;
}

static void Naive_MedianCopySort(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 11u);
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> w = data;
        state.ResumeTiming();
        std::vector<double> dv(w.begin(), w.end());
        benchmark::DoNotOptimize(naive_median(dv));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_MedianCopySort)->Arg(1'000)->Arg(20'000);

static void Linq_Median(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 11u);
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> w = data;
        state.ResumeTiming();
        double const m = qb::linq::from(w).median();
        benchmark::DoNotOptimize(&m);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_Median)->Arg(1'000)->Arg(20'000);

/// Same rank interpolation as \c percentile_by (reference for \c p = 75).
static double naive_percentile_inplace(std::vector<double>& keys, double p)
{
    std::sort(keys.begin(), keys.end());
    std::size_t const n = keys.size();
    if (n == 1)
        return keys[0];
    double const pc = std::clamp(p, 0.0, 100.0);
    double const rank = (pc / 100.0) * static_cast<double>(n - 1);
    std::size_t const i = static_cast<std::size_t>(rank);
    double const frac = rank - static_cast<double>(i);
    if (i + 1 >= n)
        return keys[n - 1];
    return keys[i] * (1.0 - frac) + keys[i + 1] * frac;
}

static void Naive_Percentile75CopySort(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 19u);
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<double> keys(data.begin(), data.end());
        state.ResumeTiming();
        double const q = naive_percentile_inplace(keys, 75.0);
        benchmark::DoNotOptimize(&q);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_Percentile75CopySort)->Arg(1'000)->Arg(20'000);

static void Linq_Percentile75(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<int> data(n);
    qb_linq_bench::fill_random(data, 19u);
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> w = data;
        state.ResumeTiming();
        double const q = qb::linq::from(w).percentile(75.0);
        benchmark::DoNotOptimize(&q);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_Percentile75)->Arg(1'000)->Arg(20'000);

static void Naive_VariancePopulation(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<double> data(n);
    for (std::size_t i = 0; i < n; ++i)
        data[i] = static_cast<double>(i % 101);
    for (auto _ : state) {
        double sum = 0;
        for (double x : data)
            sum += x;
        double const mean = sum / static_cast<double>(n);
        double acc = 0;
        for (double x : data) {
            double const d = x - mean;
            acc += d * d;
        }
        benchmark::DoNotOptimize(&acc);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Naive_VariancePopulation)->Arg(10'000)->Arg(100'000);

static void Linq_VariancePopulation(benchmark::State& state)
{
    auto const n = static_cast<std::size_t>(state.range(0));
    std::vector<double> data(n);
    for (std::size_t i = 0; i < n; ++i)
        data[i] = static_cast<double>(i % 101);
    for (auto _ : state) {
        double const v = qb::linq::from(data).variance_population();
        benchmark::DoNotOptimize(&v);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(n));
}
BENCHMARK(Linq_VariancePopulation)->Arg(10'000)->Arg(100'000);
