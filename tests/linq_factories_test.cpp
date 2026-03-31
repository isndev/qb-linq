/// @file linq_factories_test.cpp
/// @brief Entry factories: from, as_enumerable, iota, empty, once, repeat.

#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(FactoryFrom, SeesLiveContainerRange)
{
    // `from` stores iterators; growing a vector may invalidate them — do not mutate after `from`.
    std::vector<int> data{1, 2, 3};
    auto e = qb::linq::from(data);
    EXPECT_EQ(e.long_count(), 3u);
    EXPECT_TRUE(e.sequence_equal(data));
}

TEST(FactoryAsEnumerable, IdempotentOnEnumerable)
{
    std::vector<int> storage{1, 2};
    auto a = qb::linq::from(storage);
    auto b = qb::linq::as_enumerable(a);
    EXPECT_TRUE(b.sequence_equal(storage));
}

TEST(FactoryIota, HalfOpenRange)
{
    std::vector<int> out;
    for (int x : qb::linq::iota(0, 3))
        out.push_back(x);
    std::vector<int> const expect{0, 1, 2};
    EXPECT_EQ(out, expect);
}

TEST(FactoryEmpty, NoElements)
{
    auto e = qb::linq::empty<int>();
    EXPECT_FALSE(e.any());
    EXPECT_EQ(e.sum(), 0);
}

TEST(FactoryOnce, SingleElement)
{
    auto e = qb::linq::once(42);
    EXPECT_EQ(e.single(), 42);
}

TEST(FactoryRepeat, CountAndSum)
{
    auto e = qb::linq::repeat(7, 4);
    EXPECT_EQ(e.long_count(), 4u);
    EXPECT_EQ(e.sum(), 28);
}

TEST(FactoryRangeIteratorPair, SameAsFromTwoIterators)
{
    std::vector<int> data{5, 6, 7};
    auto a = qb::linq::range(data.begin(), data.end());
    auto b = qb::linq::from(data.begin(), data.end());
    EXPECT_TRUE(a.sequence_equal(b));
}

TEST(FactoryMaterialize, AliasMatchesToVector)
{
    std::vector<int> const expect{0, 1};
    EXPECT_TRUE(qb::linq::iota(0, 2).materialize().sequence_equal(expect));
}
