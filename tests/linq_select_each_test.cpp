/// @file linq_select_each_test.cpp
/// @brief select_many (tuple bundle per element), each() side effects, materialize() alias.

#include <string>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

struct record {
    int id;
    std::string name;
};

TEST(SelectManyTuple, MultipleProjectionsPerRow)
{
    std::vector<record> const data{{1, "a"}, {2, "bb"}};
    auto q = qb::linq::from(data).select_many(
        [](record const& r) { return r.id; },
        [](record const& r) { return r.name.size(); });
    std::vector<std::tuple<int, std::size_t>> got;
    for (auto const& t : q)
        got.push_back(t);
    ASSERT_EQ(got.size(), 2u);
    EXPECT_EQ(std::get<0>(got[0]), 1);
    EXPECT_EQ(std::get<1>(got[0]), 1u);
    EXPECT_EQ(std::get<0>(got[1]), 2);
    EXPECT_EQ(std::get<1>(got[1]), 2u);
}

TEST(EachSideEffect, VisitsEveryElementInOrder)
{
    std::vector<int> const data{1, 2, 3};
    std::vector<int> seen;
    auto const e = qb::linq::from(data).each([&seen](int x) { seen.push_back(x); });
    EXPECT_EQ(seen.size(), 3u);
    EXPECT_EQ(seen[0], 1);
    EXPECT_EQ(seen[2], 3);
    EXPECT_EQ(e.sum(), 6);
}

/** `each` runs eagerly before returning; in-place mutation must work without using the returned `enumerable`. */
TEST(EachSideEffect, InPlaceMutationDiscardingReturn)
{
    std::vector<int> data{1, 2, 3, 4};
    qb::linq::from(data).each([](int& x) { x *= 2; });
    std::vector<int> const expect{2, 4, 6, 8};
    EXPECT_EQ(data, expect);
}

TEST(MaterializeAlias, SameAsToVector)
{
    std::vector<int> const data{4, 5, 6};
    auto a = qb::linq::from(data).materialize();
    auto b = qb::linq::from(data).to_vector();
    EXPECT_TRUE(qb::linq::from(a).sequence_equal(b));
}
