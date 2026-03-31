/// @file linq_search_comparison_test.cpp
/// @brief index_of / last_index_of with custom equality; first_or_default_if positive path.

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(SearchIndexOf, CustomEqFindsStruct)
{
    struct item {
        int id;
        char tag;
    };
    std::vector<item> const data{{1, 'a'}, {2, 'b'}, {3, 'a'}};
    auto eq = [](item const& x, item const& y) { return x.id == y.id; };
    item const key{2, 'z'};
    auto i = qb::linq::from(data).index_of(key, eq);
    ASSERT_TRUE(i.has_value());
    EXPECT_EQ(*i, 1u);
}

TEST(SearchLastIndexOf, CustomEqRightmost)
{
    struct item {
        int k;
    };
    std::vector<item> const data{{1}, {2}, {2}, {3}};
    auto eq = [](item const& x, item const& y) { return x.k == y.k; };
    item const key{2};
    auto i = qb::linq::from(data).last_index_of(key, eq);
    ASSERT_TRUE(i.has_value());
    EXPECT_EQ(*i, 2u);
}

TEST(SearchFirstOrDefaultIf, MatchReturnsValue)
{
    std::vector<int> const data{2, 4, 6};
    EXPECT_EQ(qb::linq::from(data).first_or_default_if([](int x) { return x > 3; }), 4);
}

TEST(SearchSingleOrDefaultIf, OneMatchReturnsIt)
{
    std::vector<int> const data{1, 2, 3};
    EXPECT_EQ(qb::linq::from(data).single_or_default_if([](int x) { return x == 2; }), 2);
}

TEST(SearchContains, ValueNotPresent)
{
    std::vector<int> const data{1, 2, 3};
    EXPECT_FALSE(qb::linq::from(data).contains(99));
}

TEST(SearchLongCountIf, MatchesCountIf)
{
    std::vector<int> const data{1, 2, 3, 4};
    auto pred = [](int x) { return x % 2 == 0; };
    EXPECT_EQ(qb::linq::from(data).long_count_if(pred), qb::linq::from(data).count_if(pred));
}
