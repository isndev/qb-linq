/// @file linq_surfaces_test.cpp
/// @brief High-value API surfaces that are easy to regress: custom comparators, take edge cases,
///        zip stability, join empty result, default_if_empty(), percentile_by empty.

#include <cmath>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(SurfaceMinMaxComp, MinMaxByAbsoluteValue)
{
    std::vector<int> const data{-5, 2, -3};
    auto const q = qb::linq::from(data);
    auto const abs_less = [](int a, int b) { return std::abs(a) < std::abs(b); };
    EXPECT_EQ(q.min(abs_less), 2);
    EXPECT_EQ(q.max(abs_less), -5);
    auto const mm = q.min_max(abs_less);
    EXPECT_EQ(mm.first, 2);
    EXPECT_EQ(mm.second, -5);
}

TEST(SurfaceReduce, SingleElementReturnsValue)
{
    std::vector<int> const one{99};
    EXPECT_EQ(qb::linq::from(one).reduce([](int a, int b) { return a + b; }), 99);
}

TEST(SurfaceTake, IntMinYieldsNoElements)
{
    std::vector<int> const data{1, 2, 3};
    // PTRDIFF_MIN is the only value whose negation overflows → yields 0
    EXPECT_EQ(qb::linq::from(data).take(std::numeric_limits<std::ptrdiff_t>::min()).long_count(), 0u);
    // INT_MIN is a valid magnitude now (2147483648); taken from 3 elements → 3
    EXPECT_EQ(qb::linq::from(data).take(std::numeric_limits<int>::min()).long_count(), 3u);
}

TEST(SurfaceZip, TwoFullPassesMatch)
{
    std::vector<int> const a{1, 2, 3};
    std::vector<int> const b{10, 20, 30};
    auto const z = qb::linq::from(a).zip(b);
    std::vector<std::pair<int, int>> first;
    for (auto const& pr : z)
        first.emplace_back(pr.first, pr.second);
    std::vector<std::pair<int, int>> second;
    for (auto const& pr : z)
        second.emplace_back(pr.first, pr.second);
    ASSERT_EQ(first.size(), 3u);
    EXPECT_EQ(first, second);
}

TEST(SurfaceDefaultIfEmpty, ValueInitializedWhenNoArgument)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).default_if_empty().single(), 0);
}

TEST(SurfaceTake, IntMaxTakesEntireShortSequence)
{
    std::vector<int> const data{1, 2, 3};
    EXPECT_EQ(qb::linq::from(data).take(std::numeric_limits<int>::max()).long_count(), 3u);
    EXPECT_TRUE(qb::linq::from(data).take(std::numeric_limits<int>::max()).sequence_equal(data));
}

TEST(SurfacePercentileBy, EmptyThrows)
{
    struct row {
        double x;
    };
    std::vector<row> const empty;
    EXPECT_THROW((void)qb::linq::from(empty).percentile_by(50.0, [](row const& r) { return r.x; }),
        std::out_of_range);
}

TEST(SurfaceMedianBy, EmptyThrows)
{
    struct row {
        int v;
    };
    std::vector<row> const empty;
    EXPECT_THROW((void)qb::linq::from(empty).median_by([](row const& r) { return r.v; }), std::out_of_range);
}

TEST(SurfaceAggregate, EmptyReturnsSeed)
{
    std::vector<int> const empty;
    int const out = qb::linq::from(empty).aggregate(42, [](int acc, int x) { return acc + x; });
    EXPECT_EQ(out, 42);
}
