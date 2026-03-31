/// @file linq_order_group_lookup_test.cpp
/// @brief order_by (multi-key), desc(), group_by / to_lookup, operator[] on grouped map.

#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(OrderByMultiKey, StableLexicographicTieBreak)
{
    struct row {
        int a;
        int b;
    };
    std::vector<row> data{{1, 2}, {1, 0}, {0, 5}, {1, 1}};
    auto sorted = qb::linq::from(data)
                      .order_by(qb::linq::asc([](row const& r) { return r.a; }),
                          qb::linq::asc([](row const& r) { return r.b; }))
                      .to_vector();
    ASSERT_EQ(sorted.long_count(), 4u);
    EXPECT_EQ(sorted.element_at(0).a, 0);
    EXPECT_EQ(sorted.element_at(1).b, 0);
    EXPECT_EQ(sorted.element_at(2).b, 1);
    EXPECT_EQ(sorted.element_at(3).b, 2);
}

TEST(OrderByDesc, ReversesSortedVectorView)
{
    std::vector<int> data{1, 2, 3, 4};
    auto rev = qb::linq::from(data).order_by(qb::linq::asc([](int x) { return x; })).desc().to_vector();
    std::vector<int> const expect{4, 3, 2, 1};
    std::vector<int> flat;
    for (int x : rev)
        flat.push_back(x);
    EXPECT_EQ(flat, expect);
}

TEST(GroupBySingleKey, BucketsByParity)
{
    std::vector<int> const data{1, 2, 3, 4, 5};
    auto g = qb::linq::from(data).group_by([](int x) { return x % 2; });
    auto& bucket0 = g[0];
    EXPECT_EQ(qb::linq::from(bucket0).long_count(), 2u);
    EXPECT_EQ(qb::linq::from(bucket0).sum(), 2 + 4);
    auto& bucket1 = g[1];
    EXPECT_EQ(qb::linq::from(bucket1).sum(), 1 + 3 + 5);
}

TEST(ToLookup, SameBucketsAsSingleKeyGroupBy)
{
    std::vector<int> const data{10, 20, 11};
    auto a = qb::linq::from(data).group_by([](int x) { return x / 10; });
    auto b = qb::linq::from(data).to_lookup([](int x) { return x / 10; });
    EXPECT_EQ(qb::linq::from(a[1]).sum(), qb::linq::from(b[1]).sum());
    EXPECT_EQ(qb::linq::from(a[2]).sum(), qb::linq::from(b[2]).sum());
}

TEST(GroupByTwoKeys, NestedBuckets)
{
    struct row {
        int dept;
        int kind;
        std::string label;
    };
    std::vector<row> const data{
        {1, 0, "a"},
        {1, 0, "b"},
        {1, 1, "c"},
        {2, 0, "d"},
    };
    auto g = qb::linq::from(data).group_by(
        [](row const& r) { return r.dept; },
        [](row const& r) { return r.kind; });
    auto const& d1 = g[1];
    EXPECT_EQ(qb::linq::from(d1.at(0)).long_count(), 2u);
    EXPECT_EQ(qb::linq::from(d1.at(1)).long_count(), 1u);
    EXPECT_EQ(qb::linq::from(d1.at(1)).first().label, "c");
    auto const& d2 = g[2];
    EXPECT_EQ(qb::linq::from(d2.at(0)).long_count(), 1u);
}

TEST(GroupByEmpty, NoBuckets)
{
    std::vector<int> const empty;
    auto g = qb::linq::from(empty).group_by([](int x) { return x; });
    EXPECT_EQ(g.long_count(), 0u);
}
