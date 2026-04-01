/// @file linq_factories_test.cpp
/// @brief Entry factories: from, as_enumerable, iota, empty, once, repeat.

#include <cctype>
#include <string>
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

/** Named storage must stay in scope for lazy pipelines (`from` is non-owning). */
TEST(FactoryFrom, NamedContainerSurvivesLongLazyPipeline)
{
    std::vector<int> data;
    for (int i = 0; i < 40; ++i)
        data.push_back(i);
    auto q = qb::linq::from(data)
                 .where([](int x) { return x % 3 == 1; })
                 .select([](int x) { return x * 2; })
                 .skip(2)
                 .take(5);
    auto a = q.to_vector();
    auto b = q.to_vector();
    EXPECT_TRUE(a.sequence_equal(b));
    EXPECT_EQ(a.long_count(), 5u);
}

TEST(FactoryFrom, StdStringAsCharSequence)
{
    std::string const s{"hello"};
    auto upper = qb::linq::from(s).select([](char c) { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); });
    std::string out;
    for (char c : upper)
        out.push_back(c);
    EXPECT_EQ(out, "HELLO");
}

/** A temporary used only inside one full-expression that ends in `to_vector()` stays alive for the whole evaluation. */
TEST(FactoryFrom, PrvalueConsumedInSingleStatementToVector)
{
    auto make = [] { return std::vector<int>{1, 2, 3, 4, 5, 6}; };
    auto mat = qb::linq::from(make())
                   .where([](int x) { return x > 2; })
                   .select([](int x) { return x * 2; })
                   .to_vector();
    std::vector<int> const expect{6, 8, 10, 12};
    EXPECT_TRUE(qb::linq::from(mat).sequence_equal(expect));
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
