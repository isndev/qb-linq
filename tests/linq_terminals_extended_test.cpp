/// @file linq_terminals_extended_test.cpp
/// @brief Extended terminals: single*, or_default variants, index_of, min/max_by, comparators,
///        aggregate/fold, contains with equality, any/count, sequence_equal with comparator.

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(TerminalSingle, EmptyThrows)
{
    std::vector<int> const empty;
    EXPECT_THROW((void)qb::linq::from(empty).single(), std::out_of_range);
}

TEST(TerminalSingle, MultipleThrows)
{
    std::vector<int> const data{1, 2};
    EXPECT_THROW((void)qb::linq::from(data).single(), std::out_of_range);
}

TEST(TerminalSingle, OneElementOk)
{
    EXPECT_EQ(qb::linq::once(42).single(), 42);
}

TEST(TerminalSingleOrDefault, EmptyOrMultiReturnsDefault)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).single_or_default(), 0);

    std::vector<int> const two{1, 2};
    EXPECT_EQ(qb::linq::from(two).single_or_default(), 0);
}

TEST(TerminalSingleIf, ExactlyOneMatch)
{
    std::vector<int> const data{2, 4, 6};
    EXPECT_EQ(qb::linq::from(data).single_if([](int x) { return x > 5; }), 6);
}

TEST(TerminalSingleIf, NoMatchThrows)
{
    std::vector<int> const data{1, 2, 3};
    EXPECT_THROW((void)qb::linq::from(data).single_if([](int x) { return x > 10; }), std::out_of_range);
}

TEST(TerminalSingleIf, TwoMatchesThrows)
{
    std::vector<int> const data{2, 4, 6};
    EXPECT_THROW((void)qb::linq::from(data).single_if([](int x) { return x % 2 == 0; }), std::out_of_range);
}

TEST(TerminalSingleOrDefaultIf, AmbiguousReturnsDefault)
{
    std::vector<int> const data{2, 4};
    EXPECT_EQ(qb::linq::from(data).single_or_default_if([](int x) { return x % 2 == 0; }), 0);
}

TEST(TerminalFirstOrDefault, EmptySentinel)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).first_or_default(), 0);
}

TEST(TerminalLastOrDefault, EmptySentinel)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).last_or_default(), 0);
}

TEST(TerminalLastIf, LastMatching)
{
    std::vector<int> const data{1, 2, 3, 4, 5};
    EXPECT_EQ(qb::linq::from(data).last_if([](int x) { return x % 2 == 0; }), 4);
}

TEST(TerminalLastIf, NoMatchThrows)
{
    std::vector<int> const data{1, 3, 5};
    EXPECT_THROW((void)qb::linq::from(data).last_if([](int x) { return x % 2 == 0; }), std::out_of_range);
}

TEST(TerminalElementAtOrDefault, OutOfRangeZero)
{
    std::vector<int> const data{7};
    EXPECT_EQ(qb::linq::from(data).element_at_or_default(3), 0);
    EXPECT_EQ(qb::linq::from(data).element_at_or_default(0), 7);
}

TEST(TerminalIndexOf, FoundAndMissing)
{
    std::vector<int> const data{10, 20, 30, 20};
    auto const i = qb::linq::from(data).index_of(20);
    ASSERT_TRUE(i.has_value());
    EXPECT_EQ(*i, 1u);
    EXPECT_FALSE(qb::linq::from(data).index_of(99).has_value());
}

TEST(TerminalLastIndexOf, RightmostOccurrence)
{
    std::vector<int> const data{10, 20, 30, 20};
    auto const i = qb::linq::from(data).last_index_of(20);
    ASSERT_TRUE(i.has_value());
    EXPECT_EQ(*i, 3u);
}

TEST(TerminalContainsCustomEq, CaseInsensitiveChar)
{
    std::vector<char> const data{'A', 'B'};
    auto eq = [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    };
    EXPECT_TRUE(qb::linq::from(data).contains('a', eq));
    EXPECT_FALSE(qb::linq::from(data).contains('z', eq));
}

TEST(TerminalMinMaxCustomComp, AbsoluteOrder)
{
    std::vector<int> const data{-3, 2, -1};
    auto abs_less = [](int a, int b) { return std::abs(a) < std::abs(b); };
    EXPECT_EQ(qb::linq::from(data).min(abs_less), -1);
    EXPECT_EQ(qb::linq::from(data).max(abs_less), -3);
    auto mm = qb::linq::from(data).min_max(abs_less);
    EXPECT_EQ(mm.first, -1);
    EXPECT_EQ(mm.second, -3);
}

TEST(TerminalMinMaxOrDefault, Empty)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).min_or_default(), 0);
    EXPECT_EQ(qb::linq::from(empty).max_or_default(), 0);
}

TEST(TerminalMinByMaxBy, ByProjection)
{
    struct row {
        std::string name;
        int score;
    };
    std::vector<row> const data{{"a", 5}, {"b", 9}, {"c", 2}};
    auto const q = qb::linq::from(data);
    EXPECT_EQ(q.min_by([](row const& r) { return r.score; }).name, std::string{"c"});
    EXPECT_EQ(q.max_by([](row const& r) { return r.score; }).name, std::string{"b"});
}

TEST(TerminalMinByMaxBy, EmptyThrows)
{
    std::vector<int> const empty;
    EXPECT_THROW((void)qb::linq::from(empty).min_by([](int x) { return x; }), std::out_of_range);
    EXPECT_THROW((void)qb::linq::from(empty).max_by([](int x) { return x; }), std::out_of_range);
}

TEST(TerminalMinByOrDefault, Empty)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).min_by_or_default([](int x) { return x; }), 0);
}

TEST(TerminalAggregateFold, StringConcatMatchesStd)
{
    std::vector<std::string> const parts{"a", "b", "c"};
    std::string const a = qb::linq::from(parts).aggregate(std::string{}, [](std::string acc, std::string const& s) {
        return std::move(acc) + s;
    });
    std::string const f = qb::linq::from(parts).fold(std::string{}, [](std::string acc, std::string const& s) {
        return std::move(acc) + s;
    });
    EXPECT_EQ(a, "abc");
    EXPECT_EQ(f, "abc");
}

TEST(TerminalAnyCount, EmptyAndNonEmpty)
{
    std::vector<int> const empty;
    EXPECT_FALSE(qb::linq::from(empty).any());
    EXPECT_EQ(qb::linq::from(empty).count(), 0u);

    std::vector<int> const data{0};
    EXPECT_TRUE(qb::linq::from(data).any());
    EXPECT_EQ(qb::linq::from(data).count(), 1u);
}

TEST(TerminalSequenceEqualComp, ModComparator)
{
    std::vector<int> const a{2, 5, 8};
    std::vector<int> const b{5, 2, 11};
    auto same_mod3 = [](int x, int y) { return (x % 3 + 3) % 3 == (y % 3 + 3) % 3; };
    EXPECT_TRUE(qb::linq::from(a).sequence_equal(b, same_mod3));
    std::vector<int> const c{2, 5, 9};
    EXPECT_FALSE(qb::linq::from(a).sequence_equal(c, same_mod3));
}

TEST(TerminalFirstOrDefaultIf, NoMatchDefault)
{
    std::vector<int> const data{1, 3, 5};
    EXPECT_EQ(qb::linq::from(data).first_or_default_if([](int x) { return x % 2 == 0; }), 0);
    EXPECT_EQ(qb::linq::from(data).first_or_default_if([](int x) { return x > 2; }), 3);
}

TEST(TerminalLastOrDefaultIf, TracksLastMatch)
{
    std::vector<int> const data{1, 2, 3, 4};
    EXPECT_EQ(qb::linq::from(data).last_or_default_if([](int x) { return x % 2 == 0; }), 4);
}
