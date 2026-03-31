/// @file linq_views_compositional_test.cpp
/// @brief Lazy compositional views: concat, zip, scan, chunk, stride, append/prepend, reverse,
///        skip_while, take_while, default_if_empty, distinct_by.

#include <deque>
#include <string>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

namespace {

std::vector<int> iota_n(int n)
{
    std::vector<int> v(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i)
        v[static_cast<std::size_t>(i)] = i;
    return v;
}

} // namespace

TEST(ViewsConcat, MatchesManualAppendOrder)
{
    std::vector<int> const a{1, 2};
    std::vector<int> const b{3, 4, 5};
    auto out = qb::linq::from(a).concat(b).to_vector();
    std::vector<int> const expect{1, 2, 3, 4, 5};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
}

TEST(ViewsZip, PairsEqualLengthRanges)
{
    std::vector<int> const x{10, 20, 30};
    std::vector<char> const y{'a', 'b', 'c'};
    std::vector<std::pair<int, char>> got;
    for (auto const& pr : qb::linq::from(x).zip(y))
        got.push_back(pr);
    ASSERT_EQ(got.size(), 3u);
    EXPECT_EQ(got[0].first, 10);
    EXPECT_EQ(got[0].second, 'a');
    EXPECT_EQ(got[2].first, 30);
    EXPECT_EQ(got[2].second, 'c');
}

TEST(ViewsDefaultIfEmpty, InsertsSentinelOnlyWhenEmpty)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).default_if_empty(-1).single(), -1);

    std::vector<int> const data{5};
    EXPECT_EQ(qb::linq::from(data).default_if_empty(-1).single(), 5);
}

TEST(ViewsDefaultIfEmptyValueInit, UsesDefaultConstructedValue)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).default_if_empty().single(), 0);
}

TEST(ViewsScan, RunningTotalsMatchPartialSums)
{
    std::vector<int> const data{1, 2, 3, 4};
    std::vector<int> got;
    for (int x : qb::linq::from(data).scan(0, [](int acc, int v) { return acc + v; }))
        got.push_back(x);
    std::vector<int> const expect{1, 3, 6, 10};
    EXPECT_EQ(got, expect);
}

TEST(ViewsScan, EmptyYieldsNoOutput)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).scan(0, [](int a, int b) { return a + b; }).long_count(), 0u);
}

TEST(ViewsChunk, GroupsConsecutiveElements)
{
    std::vector<int> const data{1, 2, 3, 4, 5};
    auto chunks = qb::linq::from(data).chunk(2).to_vector();
    ASSERT_EQ(chunks.long_count(), 3u);
    EXPECT_TRUE(qb::linq::from(chunks.element_at(0)).sequence_equal(std::vector<int>{1, 2}));
    EXPECT_TRUE(qb::linq::from(chunks.element_at(1)).sequence_equal(std::vector<int>{3, 4}));
    EXPECT_TRUE(qb::linq::from(chunks.element_at(2)).sequence_equal(std::vector<int>{5}));
}

TEST(ViewsChunk, SizeZeroClampedToOne)
{
    std::vector<int> const data{7, 8};
    auto chunks = qb::linq::from(data).chunk(0).to_vector();
    ASSERT_EQ(chunks.long_count(), 2u);
}

TEST(ViewsStride, EveryNthElement)
{
    std::vector<int> const data{0, 1, 2, 3, 4, 5, 6};
    auto out = qb::linq::from(data).stride(3).to_vector();
    std::vector<int> const expect{0, 3, 6};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
}

TEST(ViewsStride, ZeroStepClampedToOne)
{
    std::vector<int> const data{1, 2, 3};
    auto out = qb::linq::from(data).stride(0).to_vector();
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(data));
}

TEST(ViewsAppendPrepend, Boundaries)
{
    std::vector<int> const data{2, 3};
    auto a = qb::linq::from(data).prepend(1).append(4).to_vector();
    std::vector<int> const expect{1, 2, 3, 4};
    EXPECT_TRUE(qb::linq::from(a).sequence_equal(expect));
}

TEST(ViewsReverse, MatchesStdReverseCopy)
{
    std::vector<int> const data{1, 2, 3, 4};
    auto rev = qb::linq::from(data).reverse().to_vector();
    std::vector<int> const expect{4, 3, 2, 1};
    EXPECT_TRUE(qb::linq::from(rev).sequence_equal(expect));
}

TEST(ViewsSkipWhile, DropsLeadingPredicateTrue)
{
    std::vector<int> const data{0, 0, 1, 2, 3};
    auto out = qb::linq::from(data).skip_while([](int x) { return x == 0; }).to_vector();
    std::vector<int> const expect{1, 2, 3};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
}

TEST(ViewsTakeWhile, StopsAtFirstFalse)
{
    std::vector<int> const data{2, 4, 5, 6};
    auto out = qb::linq::from(data).take_while([](int x) { return x % 2 == 0; }).to_vector();
    std::vector<int> const expect{2, 4};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
}

TEST(ViewsDistinctBy, KeyCollisionCollapses)
{
    std::vector<std::string> const data{"a", "bb", "ccc", "xx"};
    auto out = qb::linq::from(data).distinct_by([](std::string const& s) { return s.size(); }).to_vector();
    ASSERT_EQ(out.long_count(), 3u);
    EXPECT_TRUE(qb::linq::from(out).contains(std::string{"a"}));
    EXPECT_TRUE(qb::linq::from(out).contains(std::string{"bb"}));
    EXPECT_TRUE(qb::linq::from(out).contains(std::string{"ccc"}));
}

TEST(ViewsRepeatIteration, SecondPassMatchesFirst)
{
    std::vector<int> const data = iota_n(20);
    auto pipe = qb::linq::from(data).where([](int x) { return x % 2 != 0; }).select([](int x) { return x * 10; });
    auto const a = pipe.to_vector();
    auto const b = pipe.to_vector();
    EXPECT_TRUE(qb::linq::from(a).sequence_equal(b));
}

TEST(ViewsEnumerateAfterSelect, IndexTracksOutputOrder)
{
    std::vector<int> const data{10, 20};
    std::size_t i = 0;
    for (auto const& pr : qb::linq::from(data).select([](int x) { return x / 10; }).enumerate()) {
        EXPECT_EQ(pr.first, i);
        EXPECT_EQ(pr.second, (i == 0 ? 1 : 2));
        ++i;
    }
    EXPECT_EQ(i, 2u);
}

TEST(ViewsDequeSource, NonContiguousIterable)
{
    std::deque<int> d{9, 8, 7};
    EXPECT_EQ(qb::linq::from(d).sum(), 24);
    EXPECT_EQ(qb::linq::from(d).reverse().first(), 7);
}
