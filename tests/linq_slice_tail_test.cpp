/// @file linq_slice_tail_test.cpp
/// @brief take_last, skip_last, to_map ordering, to_unordered_set coverage.

#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(SliceTakeLast, SlidingWindowTail)
{
    std::vector<int> const data{1, 2, 3, 4, 5};
    auto t = qb::linq::from(data).take_last(3).to_vector();
    std::vector<int> const expect{3, 4, 5};
    EXPECT_TRUE(qb::linq::from(t).sequence_equal(expect));
}

TEST(SliceTakeLast, NGreaterThanSizeReturnsAll)
{
    std::vector<int> const data{1, 2};
    auto t = qb::linq::from(data).take_last(100).to_vector();
    EXPECT_TRUE(qb::linq::from(t).sequence_equal(data));
}

TEST(SliceTakeLast, ZeroNIsEmpty)
{
    std::vector<int> const data{1, 2, 3};
    auto t = qb::linq::from(data).take_last(0).to_vector();
    EXPECT_EQ(t.long_count(), 0u);
}

TEST(SliceSkipLast, DropsTrailingElements)
{
    std::vector<int> const data{1, 2, 3, 4, 5};
    auto s = qb::linq::from(data).skip_last(2).to_vector();
    std::vector<int> const expect{1, 2, 3};
    EXPECT_TRUE(qb::linq::from(s).sequence_equal(expect));
}

TEST(SliceSkipLast, NGeSizeClears)
{
    std::vector<int> const data{1, 2};
    auto s = qb::linq::from(data).skip_last(2).to_vector();
    EXPECT_EQ(s.long_count(), 0u);
}

TEST(SliceSkipLast, ZeroNNoOp)
{
    std::vector<int> const data{9, 8};
    auto s = qb::linq::from(data).skip_last(0).to_vector();
    EXPECT_TRUE(qb::linq::from(s).sequence_equal(data));
}

TEST(MaterializeToMap, SortedKeyIterationOrder)
{
    std::vector<std::pair<int, char>> const data{{3, 'c'}, {1, 'a'}, {2, 'b'}};
    auto m = qb::linq::from(data).to_map([](auto const& p) { return p.first; },
        [](auto const& p) { return p.second; });
    std::string order;
    for (auto const& pr : m)
        order.push_back(pr.second);
    EXPECT_EQ(order, "abc");
}

TEST(MaterializeToUnorderedSet, SameElementsAsSourceDistinct)
{
    std::vector<int> const data{3, 1, 3, 2, 2};
    auto us = qb::linq::from(data).to_unordered_set();
    EXPECT_EQ(us.long_count(), 3u);
    EXPECT_TRUE(qb::linq::from(us).contains(1));
    EXPECT_TRUE(qb::linq::from(us).contains(2));
    EXPECT_TRUE(qb::linq::from(us).contains(3));
}

TEST(SliceSkipLast, EmptySourceStaysEmpty)
{
    std::vector<int> const empty;
    auto s = qb::linq::from(empty).skip_last(3).to_vector();
    EXPECT_EQ(s.long_count(), 0u);
}

TEST(SliceTakeLast, EmptySourceStaysEmpty)
{
    std::vector<int> const empty;
    auto t = qb::linq::from(empty).take_last(5).to_vector();
    EXPECT_EQ(t.long_count(), 0u);
}

TEST(SliceCompose, SkipLastThenTakeLastMiddleWindow)
{
    std::vector<int> const data{1, 2, 3, 4, 5, 6, 7};
    auto w = qb::linq::from(data).skip_last(2).take_last(3).to_vector();
    std::vector<int> const expect{3, 4, 5};
    EXPECT_TRUE(qb::linq::from(w).sequence_equal(expect));
}
