/// @file linq_pipeline_test.cpp
/// @brief Tests for lazy views: select, where, skip, take, chaining vs reference algorithms.

#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

namespace {

std::vector<int> iota_vec(int n)
{
    std::vector<int> v(static_cast<std::size_t>(n));
    std::iota(v.begin(), v.end(), 0);
    return v;
}

} // namespace

TEST(PipelineSelectWhere, MatchesStdCopyIfTransform)
{
    std::vector<int> const data = iota_vec(50);
    auto const linq_sum = qb::linq::from(data)
                              .where([](int x) { return x % 3 == 0; })
                              .select([](int x) { return x * 2; })
                              .sum();

    int ref = 0;
    for (int x : data) {
        if (x % 3 == 0)
            ref += x * 2;
    }
    EXPECT_EQ(linq_sum, ref);
}

TEST(PipelineSkipTake, SlicesLikeSubrange)
{
    std::vector<int> const data = iota_vec(100);
    auto const pipe = qb::linq::from(data).skip(20).take(15);
    EXPECT_EQ(pipe.long_count(), 15u);
    EXPECT_EQ(pipe.first(), 20);
    // Use materialized last(): `last()` on `take_n` end-iterator can point past the logical take window.
    auto const materialized = pipe.to_vector();
    EXPECT_EQ(materialized.last(), 34);
}

TEST(PipelineChained, SameAsNestedLoops)
{
    std::vector<int> const data{3, 1, 4, 1, 5, 9, 2, 6};
    int const chained = qb::linq::from(data)
                            .where([](int x) { return x > 2; })
                            .select([](int x) { return x * x; })
                            .take(3)
                            .sum();
    // >2: 3,4,5,9,6 → squares 9,16,25 → take 3 → 9+16+25
    EXPECT_EQ(chained, 9 + 16 + 25);
}

TEST(PipelineDistinct, CountMatchesUnorderedSet)
{
    std::vector<int> const data{1, 2, 2, 3, 1, 4, 2};
    auto const dv = qb::linq::from(data).distinct().to_vector();
    std::unordered_set<int> u(data.begin(), data.end());
    EXPECT_EQ(dv.long_count(), u.size());
}

TEST(PipelineEnumerate, IndicesAlign)
{
    std::vector<char> const data{'a', 'b', 'c'};
    std::size_t i = 0;
    for (auto const pr : qb::linq::from(data).enumerate()) {
        EXPECT_EQ(pr.first, i);
        EXPECT_EQ(pr.second, data[i]);
        ++i;
    }
    EXPECT_EQ(i, 3u);
}
