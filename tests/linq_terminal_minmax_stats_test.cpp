/// @file linq_terminal_minmax_stats_test.cpp
/// @brief min_max(Comp), std_dev_population_by, variance_sample_by, as_enumerable rvalue.

#include <cstdlib>
#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(TerminalMinMaxComp, PairMatchesMinMaxCustom)
{
    std::vector<int> const data{-2, 5, -7};
    auto abs_less = [](int a, int b) { return std::abs(a) < std::abs(b); };
    auto mm = qb::linq::from(data).min_max(abs_less);
    EXPECT_EQ(mm.first, -2);
    EXPECT_EQ(mm.second, -7);
}

TEST(StatsStdDevPopulationBy, MatchesSqrtVarianceBy)
{
    struct row {
        double v;
    };
    std::vector<row> const data{{1.0}, {3.0}, {5.0}};
    double const vp = qb::linq::from(data).variance_population_by([](row const& r) { return r.v; });
    double const sdp = qb::linq::from(data).std_dev_population_by([](row const& r) { return r.v; });
    EXPECT_NEAR(sdp, std::sqrt(vp), 1e-9);
}

TEST(StatsVarianceSampleBy, TwoPoints)
{
    struct p {
        int id;
        double x;
    };
    std::vector<p> const data{{0, 10.0}, {1, 20.0}};
    double const vs = qb::linq::from(data).variance_sample_by([](p const& r) { return r.x; });
    EXPECT_NEAR(vs, 50.0, 1e-9);
}

TEST(FactoryAsEnumerableRvalue, MovePreservesSequence)
{
    std::vector<int> v{1, 2, 3};
    auto e = qb::linq::from(v);
    auto w = qb::linq::as_enumerable(std::move(e));
    EXPECT_TRUE(w.sequence_equal(v));
}

TEST(FactoryFromMutableContainer, SeesUpdatedElement)
{
    std::vector<int> data{1, 2, 3};
    auto e = qb::linq::from(data);
    data[1] = 99;
    EXPECT_EQ(e.element_at(1), 99);
}
