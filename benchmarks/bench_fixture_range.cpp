/**
 * @file bench_fixture_range.cpp
 * @brief Fixture-based scaling benchmarks (Naive vs Linq sum).
 */

#include <benchmark/benchmark.h>

#include <vector>

#include "detail/bench_common.hpp"
#include "qb/linq.h"

namespace {

class RandomVectorFixture : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State const& state) override
    {
        auto const n = static_cast<std::size_t>(state.range(0));
        data_.resize(n);
        qb_linq_bench::fill_random(data_, 42u);
    }

    std::vector<int> data_;
};

} // namespace

BENCHMARK_DEFINE_F(RandomVectorFixture, Linq_Sum_Fixture)(benchmark::State& state)
{
    for (auto _ : state) {
        int const s = qb::linq::from(data_).sum();
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data_.size()));
}
BENCHMARK_REGISTER_F(RandomVectorFixture, Linq_Sum_Fixture)->RangeMultiplier(2)->Range(1 << 12, 1 << 18);

BENCHMARK_DEFINE_F(RandomVectorFixture, Naive_Sum_Fixture)(benchmark::State& state)
{
    for (auto _ : state) {
        long long s = 0;
        for (int x : data_)
            s += x;
        benchmark::DoNotOptimize(&s);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data_.size()));
}
BENCHMARK_REGISTER_F(RandomVectorFixture, Naive_Sum_Fixture)->RangeMultiplier(2)->Range(1 << 12, 1 << 18);
