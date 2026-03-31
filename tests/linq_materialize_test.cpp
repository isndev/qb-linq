/// @file linq_materialize_test.cpp
/// @brief Materializing operations: order_by, maps, except / intersect, to_vector.

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(MaterializeOrderBy, SortsLexicographically)
{
    std::vector<int> data{3, 1, 4, 1, 5};
    auto sorted = qb::linq::from(data).order_by(qb::linq::asc([](int x) { return x; })).to_vector();
    ASSERT_EQ(sorted.long_count(), data.size());
    auto it = sorted.begin();
    int prev = *it++;
    for (; it != sorted.end(); ++it) {
        EXPECT_LE(prev, *it);
        prev = *it;
    }
}

TEST(MaterializeToUnorderedMap, LastWinsOnDuplicateKey)
{
    struct row {
        int k;
        std::string v;
    };
    std::vector<row> const data{{1, "a"}, {2, "b"}, {1, "c"}};
    auto m = qb::linq::from(data)
                 .to_unordered_map([](row const& r) { return r.k; }, [](row const& r) { return r.v; });
    EXPECT_EQ(m[1], "c");
    EXPECT_EQ(m[2], "b");
}

TEST(MaterializeToDictionary, DuplicateKeyThrows)
{
    std::vector<std::pair<int, int>> const data{{1, 10}, {2, 20}, {1, 99}};
    EXPECT_THROW(
        (void)qb::linq::from(data)
            .to_dictionary([](auto const& p) { return p.first; }, [](auto const& p) { return p.second; }),
        std::invalid_argument);
}

TEST(MaterializeExcept, RemovesBanned)
{
    std::vector<int> const main{1, 2, 3, 4, 5};
    std::vector<int> const ban{2, 4};
    auto out = qb::linq::from(main).except(ban).to_vector();
    std::vector<int> const expect{1, 3, 5};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
}

TEST(MaterializeIntersect, KeepsOrderOfLeft)
{
    std::vector<int> const a{1, 2, 3, 4};
    std::vector<int> const b{0, 3, 4, 5};
    auto out = qb::linq::from(a).intersect(b).to_vector();
    std::vector<int> const expect{3, 4};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
}

TEST(MaterializeToSet, UniqueSortedTree)
{
    std::vector<int> const data{3, 1, 3, 2};
    auto s = qb::linq::from(data).to_set();
    std::vector<int> const expect{1, 2, 3};
    EXPECT_TRUE(qb::linq::from(s).sequence_equal(expect));
}
