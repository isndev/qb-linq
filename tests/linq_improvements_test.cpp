/**
 * @file linq_improvements_test.cpp
 * @brief Tests for worldclass improvements: forward-safe last(), ptrdiff_t take(),
 *        default_if_empty correctness, single-pass terminals.
 */

#include <qb/linq.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <limits>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// last() / last_or_default() on forward-only iterators
// ---------------------------------------------------------------------------

TEST(ForwardLast, ConcatViewLastWorks)
{
    std::vector<int> a{1, 2, 3};
    std::vector<int> b{4, 5, 6};
    // concat_view produces forward-only iterators
    auto result = qb::linq::from(a).concat(b).last();
    EXPECT_EQ(result, 6);
}

TEST(ForwardLast, ConcatViewLastOrDefaultWorks)
{
    std::vector<int> a{10, 20};
    std::vector<int> b{30};
    auto result = qb::linq::from(a).concat(b).last_or_default();
    EXPECT_EQ(result, 30);
}

TEST(ForwardLast, ConcatViewLastOrDefaultOnEmpty)
{
    std::vector<int> a;
    std::vector<int> b;
    auto result = qb::linq::from(a).concat(b).last_or_default();
    EXPECT_EQ(result, 0);
}

TEST(ForwardLast, ConcatViewLastThrowsOnEmpty)
{
    std::vector<int> a;
    std::vector<int> b;
    EXPECT_THROW((void)qb::linq::from(a).concat(b).last(), std::out_of_range);
}

TEST(ForwardLast, ZipViewLastWorks)
{
    std::vector<int> a{1, 2, 3};
    std::vector<int> b{10, 20, 30};
    auto result = qb::linq::from(a).zip(b).last();
    EXPECT_EQ(result.first, 3);
    EXPECT_EQ(result.second, 30);
}

TEST(ForwardLast, DistinctViewLastWorks)
{
    std::vector<int> data{1, 2, 3, 2, 4};
    auto result = qb::linq::from(data).distinct().last();
    EXPECT_EQ(result, 4);
}

TEST(ForwardLast, EnumerateViewLastWorks)
{
    std::vector<int> data{10, 20, 30};
    auto result = qb::linq::from(data).enumerate().last();
    EXPECT_EQ(result.first, 2u);
    EXPECT_EQ(result.second, 30);
}

TEST(ForwardLast, ScanViewLastWorks)
{
    std::vector<int> data{1, 2, 3, 4};
    auto result = qb::linq::from(data).scan(0, [](int acc, int x) { return acc + x; }).last();
    EXPECT_EQ(result, 10);
}

TEST(ForwardLast, ChunkViewLastWorks)
{
    std::vector<int> data{1, 2, 3, 4, 5};
    auto result = qb::linq::from(data).chunk(2).last();
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0], 5);
}

TEST(ForwardLast, StrideViewLastWorks)
{
    std::vector<int> data{1, 2, 3, 4, 5, 6};
    auto result = qb::linq::from(data).stride(2).last();
    EXPECT_EQ(result, 5);
}

// ---------------------------------------------------------------------------
// ptrdiff_t take() — large values
// ---------------------------------------------------------------------------

TEST(TakePtrdiff, LargePositiveValue)
{
    std::vector<int> data{1, 2, 3};
    std::ptrdiff_t big = static_cast<std::ptrdiff_t>(std::numeric_limits<int>::max()) + 1;
    auto result = qb::linq::from(data).take(big).to_vector();
    EXPECT_TRUE(qb::linq::from(result).sequence_equal(data));
}

TEST(TakePtrdiff, LargeNegativeValue)
{
    std::vector<int> data{1, 2, 3};
    std::ptrdiff_t big = -static_cast<std::ptrdiff_t>(std::numeric_limits<int>::max()) - 1;
    auto result = qb::linq::from(data).take(big).to_vector();
    EXPECT_TRUE(qb::linq::from(result).sequence_equal(data));
}

TEST(TakePtrdiff, PtrdiffMinYieldsZero)
{
    std::vector<int> data{1, 2, 3};
    EXPECT_EQ(qb::linq::from(data).take(std::numeric_limits<std::ptrdiff_t>::min()).count(), 0u);
}

TEST(TakePtrdiff, ZeroStillYieldsNothing)
{
    std::vector<int> data{1, 2, 3};
    EXPECT_EQ(qb::linq::from(data).take(0).count(), 0u);
}

// ---------------------------------------------------------------------------
// default_if_empty — no const_cast needed
// ---------------------------------------------------------------------------

TEST(DefaultIfEmpty, NonEmptyForwardsElements)
{
    std::vector<int> data{1, 2, 3};
    auto result = qb::linq::from(data).default_if_empty(42).to_vector();
    EXPECT_TRUE(qb::linq::from(result).sequence_equal(data));
}

TEST(DefaultIfEmpty, EmptyYieldsDefault)
{
    std::vector<int> data;
    auto result = qb::linq::from(data).default_if_empty(42).to_vector();
    std::vector<int> expected{42};
    EXPECT_TRUE(qb::linq::from(result).sequence_equal(expected));
}

TEST(DefaultIfEmpty, EmptyStringYieldsDefault)
{
    std::vector<std::string> data;
    auto result = qb::linq::from(data).default_if_empty(std::string("hello")).to_vector();
    EXPECT_EQ(qb::linq::from(result).count(), 1u);
    EXPECT_EQ(qb::linq::from(result).first(), "hello");
}

// ---------------------------------------------------------------------------
// Single-pass terminal efficiency (no double begin/end)
// These test correctness — efficiency is guaranteed by the code change.
// ---------------------------------------------------------------------------

TEST(SinglePassTerminals, FirstOnLazyPipeline)
{
    std::vector<int> data{5, 10, 15};
    EXPECT_EQ(qb::linq::from(data).where([](int x) { return x > 7; }).first(), 10);
}

TEST(SinglePassTerminals, FirstOrDefaultOnEmpty)
{
    std::vector<int> data;
    EXPECT_EQ(qb::linq::from(data).first_or_default(), 0);
}

TEST(SinglePassTerminals, MinOnLazy)
{
    std::vector<int> data{5, 1, 3, 2, 4};
    EXPECT_EQ(qb::linq::from(data).select([](int x) { return x * 2; }).min(), 2);
}

TEST(SinglePassTerminals, MaxOnLazy)
{
    std::vector<int> data{5, 1, 3, 2, 4};
    EXPECT_EQ(qb::linq::from(data).select([](int x) { return x * 2; }).max(), 10);
}

TEST(SinglePassTerminals, MinMaxOnLazy)
{
    std::vector<int> data{5, 1, 3, 2, 4};
    auto [lo, hi] = qb::linq::from(data).min_max();
    EXPECT_EQ(lo, 1);
    EXPECT_EQ(hi, 5);
}

TEST(SinglePassTerminals, MinOrDefaultOnEmpty)
{
    std::vector<int> data;
    EXPECT_EQ(qb::linq::from(data).min_or_default(), 0);
}

TEST(SinglePassTerminals, MaxOrDefaultOnEmpty)
{
    std::vector<int> data;
    EXPECT_EQ(qb::linq::from(data).max_or_default(), 0);
}

TEST(SinglePassTerminals, MinByOrDefaultOnEmpty)
{
    std::vector<std::string> data;
    auto result = qb::linq::from(data).min_by_or_default([](std::string const& s) { return s.size(); });
    EXPECT_EQ(result, "");
}

TEST(SinglePassTerminals, MaxByOrDefaultOnEmpty)
{
    std::vector<std::string> data;
    auto result = qb::linq::from(data).max_by_or_default([](std::string const& s) { return s.size(); });
    EXPECT_EQ(result, "");
}

TEST(SinglePassTerminals, MinByOrDefaultOnNonEmpty)
{
    std::vector<std::string> data{"hello", "hi", "hey"};
    auto result = qb::linq::from(data).min_by_or_default([](std::string const& s) { return s.size(); });
    EXPECT_EQ(result, "hi");
}

TEST(SinglePassTerminals, MaxByOrDefaultOnNonEmpty)
{
    std::vector<std::string> data{"hello", "hi", "hey"};
    auto result = qb::linq::from(data).max_by_or_default([](std::string const& s) { return s.size(); });
    EXPECT_EQ(result, "hello");
}
