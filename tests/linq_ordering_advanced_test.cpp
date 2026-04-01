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

/** Equal sort keys must keep first-seen order (matches .NET `OrderBy` stability; `order_by` uses `stable_sort`). */
TEST(OrderAdvanced, StableForDuplicateSortKey)
{
    struct item {
        int key;
        int seq;
    };
    std::vector<item> data{{1, 0}, {1, 1}, {2, 2}, {1, 3}, {0, 4}};
    auto sorted = qb::linq::from(data).order_by(qb::linq::asc([](item const& x) { return x.key; })).to_vector();
    ASSERT_EQ(sorted.long_count(), 5u);
    std::vector<int> seq_order;
    for (auto const& x : sorted)
        seq_order.push_back(x.seq);
    EXPECT_EQ(seq_order, (std::vector<int>{4, 0, 1, 3, 2}));
}

TEST(OrderAdvanced, PreSortedInputYieldsSameOrderedProjectionAsShuffled)
{
    struct row {
        int dept;
        int id;
    };
    std::vector<row> const sorted_in{{1, 9}, {1, 5}, {1, 3}, {2, 10}, {2, 7}, {2, 1}};
    std::vector<row> shuffled{{2, 10}, {1, 5}, {1, 3}, {2, 7}, {1, 9}, {2, 1}};
    auto proj = [](std::vector<row> const& rows) {
        return qb::linq::from(rows)
            .order_by(
                qb::linq::asc([](row const& r) { return r.dept; }),
                qb::linq::desc([](row const& r) { return r.id; }))
            .select([](row const& r) { return r.dept * 100 + r.id; })
            .to_vector();
    };
    EXPECT_TRUE(qb::linq::from(proj(sorted_in)).sequence_equal(proj(shuffled)));
}
