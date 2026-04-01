/// @file linq_boundaries_empty_test.cpp
/// @brief Empty sequences, skip/take edge sizes, vacuous predicates, factory edge counts.

#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(BoundariesEmpty, AllIfVacuousTrue)
{
    std::vector<int> const empty;
    EXPECT_TRUE(qb::linq::from(empty).all_if([](int) { return false; }));
}

TEST(BoundariesEmpty, NoneIfVacuousTrue)
{
    std::vector<int> const empty;
    EXPECT_TRUE(qb::linq::from(empty).none_if([](int) { return true; }));
}

TEST(BoundariesEmpty, AnyIfFalse)
{
    std::vector<int> const empty;
    EXPECT_FALSE(qb::linq::from(empty).any_if([](int) { return true; }));
}

TEST(BoundariesEmpty, CountIfZero)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).count_if([](int) { return true; }), 0u);
}

TEST(BoundariesEmpty, SumIfZero)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).sum_if([](int) { return true; }), 0);
}

TEST(BoundariesEmpty, OrderByToVectorEmpty)
{
    std::vector<int> const empty;
    auto v = qb::linq::from(empty).order_by(qb::linq::asc([](int x) { return x; })).to_vector();
    EXPECT_EQ(v.long_count(), 0u);
}

TEST(BoundariesEmpty, DistinctEmpty)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).distinct().long_count(), 0u);
}

TEST(BoundariesEmpty, GroupByEmptySingleKey)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).group_by([](int x) { return x; }).long_count(), 0u);
}

TEST(BoundariesFactory, RepeatZeroTimes)
{
    auto e = qb::linq::repeat(99, 0);
    EXPECT_EQ(e.long_count(), 0u);
}

TEST(BoundariesFactory, IotaEmptyRange)
{
    EXPECT_FALSE(qb::linq::iota(5, 5).any());
}

TEST(BoundariesFactory, EmptySumDefault)
{
    EXPECT_EQ(qb::linq::empty<int>().sum(), 0);
}

TEST(BoundariesSkip, SkipPastEndIsEmpty)
{
    std::vector<int> const data{1, 2};
    auto s = qb::linq::from(data).skip(10).to_vector();
    EXPECT_EQ(s.long_count(), 0u);
}

TEST(BoundariesSkip, SkipZeroNoOp)
{
    std::vector<int> const data{3, 4};
    EXPECT_TRUE(qb::linq::from(data).skip(0).sequence_equal(data));
}

TEST(BoundariesTake, TakeZeroEmpty)
{
    std::vector<int> const data{1, 2, 3};
    EXPECT_EQ(qb::linq::from(data).take(0).long_count(), 0u);
}

TEST(BoundariesTake, TakeMoreThanSizeGetsAll)
{
    std::vector<int> const data{7, 8};
    EXPECT_TRUE(qb::linq::from(data).take(100).sequence_equal(data));
}

TEST(BoundariesSkipWhile, NeverFalseYieldsEmpty)
{
    std::vector<int> const data{1, 2, 3};
    EXPECT_EQ(qb::linq::from(data).skip_while([](int) { return true; }).long_count(), 0u);
}

TEST(BoundariesTakeWhile, NeverTrueYieldsEmpty)
{
    std::vector<int> const data{1, 2, 3};
    EXPECT_EQ(qb::linq::from(data).take_while([](int) { return false; }).long_count(), 0u);
}

TEST(BoundariesSequenceEqual, DifferentLengthsFalse)
{
    std::vector<int> const a{1, 2, 3};
    std::vector<int> const b{1, 2};
    EXPECT_FALSE(qb::linq::from(a).sequence_equal(b));
    EXPECT_FALSE(qb::linq::from(b).sequence_equal(a));
}

TEST(BoundariesSequenceEqual, PrefixFalseSameLengthMismatch)
{
    std::vector<int> const a{1, 2, 3};
    std::vector<int> const b{1, 2, 4};
    EXPECT_FALSE(qb::linq::from(a).sequence_equal(b));
}

TEST(BoundariesZip, LeftEmptyYieldsNoPairs)
{
    std::vector<int> const empty;
    std::vector<int> const rhs{1, 2};
    auto z = qb::linq::from(empty).zip(rhs);
    EXPECT_FALSE(z.any());
    EXPECT_EQ(z.long_count(), 0u);
}

TEST(BoundariesZip, RightEmptyYieldsNoPairs)
{
    std::vector<int> const lhs{1, 2};
    std::vector<int> const empty;
    auto z = qb::linq::from(lhs).zip(empty);
    EXPECT_FALSE(z.any());
    EXPECT_EQ(z.long_count(), 0u);
}

TEST(BoundariesConcat, EmptyLeftOnlyRight)
{
    std::vector<int> const empty;
    std::vector<int> const rhs{5, 6};
    auto out = qb::linq::from(empty).concat(rhs).to_vector();
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(rhs));
}

TEST(BoundariesConcat, EmptyRightOnlyLeft)
{
    std::vector<int> const lhs{1};
    std::vector<int> const empty;
    auto out = qb::linq::from(lhs).concat(empty).to_vector();
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(lhs));
}

TEST(BoundariesExcept, RemovesEverything)
{
    std::vector<int> const main{1, 2};
    std::vector<int> const ban{1, 2, 3};
    auto out = qb::linq::from(main).except(ban).to_vector();
    EXPECT_EQ(out.long_count(), 0u);
}

TEST(BoundariesUnion, BothEmpty)
{
    std::vector<int> const a;
    std::vector<int> const b;
    auto u = qb::linq::from(a).union_with(b).to_vector();
    EXPECT_EQ(u.long_count(), 0u);
}

TEST(BoundariesMaxByOrDefault, NonEmptyDelegates)
{
    std::vector<int> const data{3, 1, 2};
    EXPECT_EQ(qb::linq::from(data).max_by_or_default([](int x) { return x; }), 3);
}

TEST(BoundariesMinMaxOrDefault, NonEmpty)
{
    std::vector<int> const data{5, 9};
    EXPECT_EQ(qb::linq::from(data).min_or_default(), 5);
    EXPECT_EQ(qb::linq::from(data).max_or_default(), 9);
}
