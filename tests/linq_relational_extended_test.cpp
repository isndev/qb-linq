/// @file linq_relational_extended_test.cpp
/// @brief Join / group_join edge cases, duplicate keys, empty outer.

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

struct id_row {
    int id;
    std::string name;
};

struct tag_row {
    int id;
    int tag;
};

TEST(RelationalJoin, DuplicateInnerKeysProducesMultipleRows)
{
    std::vector<id_row> const outer{{1, "a"}};
    std::vector<tag_row> const inner{{1, 10}, {1, 11}};
    auto j = qb::linq::from(outer)
                 .join(inner,
                     [](id_row const& r) { return r.id; },
                     [](tag_row const& r) { return r.id; },
                     [](id_row const& o, tag_row const& i) { return std::make_pair(o.name, i.tag); });
    ASSERT_EQ(j.long_count(), 2u);
    int sum = 0;
    for (auto const& p : j)
        sum += p.second;
    EXPECT_EQ(sum, 21);
}

TEST(RelationalGroupJoin, OuterEmpty)
{
    std::vector<id_row> const outer;
    std::vector<tag_row> const inner{{1, 1}};
    auto gj = qb::linq::from(outer).group_join(inner,
        [](id_row const& r) { return r.id; },
        [](tag_row const& r) { return r.id; });
    EXPECT_EQ(gj.long_count(), 0u);
}

TEST(RelationalGroupJoin, InnerEmptyBuckets)
{
    std::vector<id_row> const outer{{1, "x"}, {2, "y"}};
    std::vector<tag_row> const inner;
    auto gj = qb::linq::from(outer).group_join(inner,
        [](id_row const& r) { return r.id; },
        [](tag_row const& r) { return r.id; });
    ASSERT_EQ(gj.long_count(), 2u);
    for (auto const& row : gj) {
        EXPECT_EQ(row.second.size(), 0u);
    }
}

TEST(RelationalUnion, AllDuplicatesCollapsesToOne)
{
    std::vector<int> const a{1, 1, 1};
    std::vector<int> const b{1};
    auto u = qb::linq::from(a).union_with(b).to_vector();
    EXPECT_EQ(u.long_count(), 1u);
    EXPECT_EQ(u.first(), 1);
}

TEST(RelationalIntersect, FullOverlap)
{
    std::vector<int> const a{1, 2, 3};
    std::vector<int> const b{3, 2, 1};
    auto out = qb::linq::from(a).intersect(b).to_vector();
    EXPECT_EQ(out.long_count(), 3u);
}
