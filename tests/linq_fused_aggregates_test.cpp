/// @file linq_fused_aggregates_test.cpp
/// @brief Fused predicate + fold terminals (single pass vs naive filter-then-fold).

#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(FusedSumIf, MatchesManualFilter)
{
    std::vector<int> const data{1, 2, 3, 4, 5, 6};
    int const fused = qb::linq::from(data).sum_if([](int x) { return x % 2 == 0; });
    int manual = 0;
    for (int x : data) {
        if (x % 2 == 0)
            manual += x;
    }
    EXPECT_EQ(fused, manual);
    EXPECT_EQ(fused, 2 + 4 + 6);
}

TEST(FusedCountIf, MatchesStdCountIfPattern)
{
    std::vector<int> const data{1, 2, 3, 4};
    auto const pred = [](int x) { return x > 2; };
    EXPECT_EQ(qb::linq::from(data).count_if(pred), 2u);
    EXPECT_EQ(qb::linq::from(data).long_count_if(pred), 2u);
}

TEST(FusedAnyAllNone, BooleanLogic)
{
    std::vector<int> const data{-1, 0, 3};
    EXPECT_TRUE(qb::linq::from(data).any_if([](int x) { return x > 0; }));
    EXPECT_FALSE(qb::linq::from(data).all_if([](int x) { return x > 0; }));
    EXPECT_TRUE(qb::linq::from(data).none_if([](int x) { return x > 100; }));
}

TEST(FusedAverageIf, FiltersBeforeMean)
{
    std::vector<int> const data{1, 2, 3, 4};
    double const a = qb::linq::from(data).average_if([](int x) { return x % 2 == 0; });
    EXPECT_DOUBLE_EQ(a, 3.0); // 2 and 4
}
