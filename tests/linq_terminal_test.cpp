/// @file linq_terminal_test.cpp
/// @brief Single-pass terminals, accessors, predicates, and error contracts (exceptions).

#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(TerminalSum, EmptyIsZeroForInt)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).sum(), 0);
}

TEST(TerminalMinMax, EmptyThrows)
{
    std::vector<int> const empty;
    EXPECT_THROW((void)qb::linq::from(empty).min(), std::out_of_range);
    EXPECT_THROW((void)qb::linq::from(empty).max(), std::out_of_range);
    EXPECT_THROW((void)qb::linq::from(empty).min_max(), std::out_of_range);
}

TEST(TerminalMinMax, MatchesStdOnData)
{
    std::vector<int> const data{3, 1, 4, 1, 5};
    auto const q = qb::linq::from(data);
    EXPECT_EQ(q.min(), 1);
    EXPECT_EQ(q.max(), 5);
    auto const mm = q.min_max();
    EXPECT_EQ(mm.first, 1);
    EXPECT_EQ(mm.second, 5);
}

TEST(TerminalFirstLast, EmptyThrows)
{
    std::vector<int> const empty;
    EXPECT_THROW((void)qb::linq::from(empty).first(), std::out_of_range);
    EXPECT_THROW((void)qb::linq::from(empty).last(), std::out_of_range);
}

TEST(TerminalFirstLast, Bounds)
{
    std::vector<int> const data{7, 8, 9};
    auto const q = qb::linq::from(data);
    EXPECT_EQ(q.first(), 7);
    EXPECT_EQ(q.last(), 9);
}

TEST(TerminalElementAt, OutOfRangeThrows)
{
    std::vector<int> const data{10, 20};
    EXPECT_THROW((void)qb::linq::from(data).element_at(2), std::out_of_range);
    EXPECT_EQ(qb::linq::from(data).element_at(1), 20);
}

TEST(TerminalAverage, EmptyThrowsNonEmptyMatches)
{
    std::vector<int> const empty;
    EXPECT_THROW((void)qb::linq::from(empty).average(), std::out_of_range);
    std::vector<int> const data{2, 4, 6};
    EXPECT_DOUBLE_EQ(qb::linq::from(data).average(), 4.0);
}

TEST(TerminalContains, SequenceEqual)
{
    std::vector<int> const a{1, 2, 3};
    std::vector<int> const b{1, 2, 3};
    std::vector<int> const c{1, 2};
    EXPECT_TRUE(qb::linq::from(a).sequence_equal(b));
    EXPECT_FALSE(qb::linq::from(a).sequence_equal(c));
    EXPECT_TRUE(qb::linq::from(a).contains(2));
    EXPECT_FALSE(qb::linq::from(a).contains(99));
}

TEST(TerminalContains, EmptyNeverContains)
{
    std::vector<int> const empty;
    EXPECT_FALSE(qb::linq::from(empty).contains(0));
    EXPECT_FALSE(qb::linq::from(empty).contains(0, [](int a, int b) { return a == b; }));
}

TEST(TerminalReduce, EmptyThrows)
{
    std::vector<int> const empty;
    EXPECT_THROW((void)qb::linq::from(empty).reduce([](int a, int b) { return a + b; }), std::out_of_range);
}

TEST(TerminalReduce, SameAsAccumulate)
{
    std::vector<int> const data{1, 2, 3, 4};
    int const r = qb::linq::from(data).reduce([](int a, int b) { return a + b; });
    EXPECT_EQ(r, 10);
}

TEST(TerminalFirstIf, ThrowsWhenNoMatch)
{
    std::vector<int> const data{1, 3, 5};
    EXPECT_THROW((void)qb::linq::from(data).first_if([](int x) { return x % 2 == 0; }), std::out_of_range);
    EXPECT_EQ(qb::linq::from(data).first_if([](int x) { return x > 2; }), 3);
}

TEST(TerminalFirstIf, EmptyThrows)
{
    std::vector<int> const empty;
    EXPECT_THROW((void)qb::linq::from(empty).first_if([](int) { return true; }), std::out_of_range);
}

TEST(TerminalLastIf, EmptyThrows)
{
    std::vector<int> const empty;
    EXPECT_THROW((void)qb::linq::from(empty).last_if([](int) { return true; }), std::out_of_range);
}

TEST(TerminalElementAt, EmptyAlwaysThrows)
{
    std::vector<int> const empty;
    EXPECT_THROW((void)qb::linq::from(empty).element_at(0), std::out_of_range);
}
