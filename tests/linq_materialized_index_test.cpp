/// @file linq_materialized_index_test.cpp
/// @brief operator[] on map-backed materialized ranges; reverse() on materialized vector.

#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(MaterializedIndex, ToMapBracketOperator)
{
    std::vector<std::pair<int, char>> const data{{2, 'b'}, {1, 'a'}};
    auto m = qb::linq::from(data).to_map([](auto const& p) { return p.first; },
        [](auto const& p) { return p.second; });
    EXPECT_EQ(m[1], 'a');
    EXPECT_EQ(m[2], 'b');
}

TEST(MaterializedIndex, ToUnorderedMapBracketOperator)
{
    std::vector<std::pair<int, int>> const data{{5, 50}, {7, 70}};
    auto m = qb::linq::from(data).to_unordered_map([](auto const& p) { return p.first; },
        [](auto const& p) { return p.second; });
    EXPECT_EQ(m[5], 50);
    EXPECT_EQ(m[7], 70);
}

TEST(MaterializedReverse, MaterializedVectorReverseView)
{
    std::vector<int> const data{1, 2, 3};
    auto rev = qb::linq::from(data).to_vector().reverse().to_vector();
    std::vector<int> const expect{3, 2, 1};
    EXPECT_TRUE(qb::linq::from(rev).sequence_equal(expect));
}

TEST(MaterializedDesc, SortedThenDescIterator)
{
    std::vector<int> data{9, 1, 8};
    auto v = qb::linq::from(data).order_by(qb::linq::asc([](int x) { return x; })).desc().to_vector();
    std::vector<int> flat;
    for (int x : v)
        flat.push_back(x);
    std::vector<int> const expect{9, 8, 1};
    EXPECT_EQ(flat, expect);
}
