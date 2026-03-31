/// @file linq_ordering_advanced_test.cpp
/// @brief order_by descending key, asc() identity on sorted materialized, make_filter custom order.

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(OrderAdvanced, OrderByDescendingKey)
{
    std::vector<int> data{1, 5, 2, 4};
    auto sorted = qb::linq::from(data).order_by(qb::linq::desc([](int x) { return x; })).to_vector();
    std::vector<int> const expect{5, 4, 2, 1};
    EXPECT_TRUE(qb::linq::from(sorted).sequence_equal(expect));
}

TEST(OrderAdvanced, AscOnMaterializedNoOpOrder)
{
    std::vector<int> data{3, 1, 2};
    auto v = qb::linq::from(data).order_by(qb::linq::asc([](int x) { return x; })).asc().to_vector();
    std::vector<int> const expect{1, 2, 3};
    EXPECT_TRUE(qb::linq::from(v).sequence_equal(expect));
}

TEST(OrderAdvanced, MakeFilterCustomComparator)
{
    struct abs_less {
        bool operator()(int lhs, int rhs) const
        {
            return std::abs(lhs) < std::abs(rhs);
        }
        bool next(int lhs, int rhs) const { return std::abs(lhs) == std::abs(rhs); }
    };

    std::vector<int> data{-3, 2, -1, 0};
    auto sorted = qb::linq::from(data)
                      .order_by(qb::linq::make_filter<abs_less>([](int x) { return x; }))
                      .to_vector();
    int prev = 0;
    for (int x : sorted) {
        EXPECT_LE(prev, std::abs(x));
        prev = std::abs(x);
    }
    EXPECT_EQ(sorted.long_count(), 4u);
}

TEST(OrderAdvanced, ThenByStyleTwoAscKeys)
{
    struct row {
        int a;
        int b;
    };
    std::vector<row> data{{1, 2}, {2, 0}, {1, 0}, {2, 1}};
    auto sorted = qb::linq::from(data)
                      .order_by(qb::linq::asc([](row const& r) { return r.a; }),
                          qb::linq::asc([](row const& r) { return r.b; }))
                      .to_vector();
    EXPECT_EQ(sorted.element_at(0).b, 0);
    EXPECT_EQ(sorted.element_at(1).b, 2);
    EXPECT_EQ(sorted.element_at(2).b, 0);
    EXPECT_EQ(sorted.element_at(3).b, 1);
}
