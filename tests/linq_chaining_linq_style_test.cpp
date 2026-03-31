/// @file linq_chaining_linq_style_test.cpp
/// @brief LINQ-style composability: long lazy chains, factories (\c iota), \c skip_while/\c take_while,
///        \c default_if_empty after filters, \c group_join with a lazy inner, \c join with a lazy outer,
///        and operators that accept another pipeline (\c zip, \c concat, \c sequence_equal, set ops).
///
/// @note For \c zip / \c concat, the inner iterator types may assign into nested iterators; use
///       stateless callables decayable to function pointers (\c +[](){}) or named functors — not
///       capturing lambdas — on the inner pipeline when it embeds \c where / \c select.

#include <numeric>
#include <string>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

namespace {

std::vector<int> iota_n(int n)
{
    std::vector<int> v(static_cast<std::size_t>(n));
    std::iota(v.begin(), v.end(), 0);
    return v;
}

} // namespace

/// Several lazy stages before a terminal (no early materialization).
TEST(ChainLinq_DeepLazyPipeline, WhereSelectSkipTakeSum)
{
    std::vector<int> const data = iota_n(20);
    int const got = qb::linq::from(data)
                        .where([](int x) { return x % 2 == 0; })
                        .select([](int x) { return x * 10; })
                        .skip(2)
                        .take(4)
                        .sum();
    // even: 0,2,4,...,18 → ×10: 0,20,40,...,180 — skip 2 → 40,60,80,100 — take 4 sum
    EXPECT_EQ(got, 40 + 60 + 80 + 100);
}

TEST(ChainLinq_DeepLazyPipeline, DistinctStrideThenOrderBy)
{
    std::vector<int> const data{5, 5, 1, 1, 9, 9, 2};
    auto const sorted = qb::linq::from(data)
                            .stride(1)
                            .distinct()
                            .order_by(qb::linq::asc([](int x) { return x; }))
                            .to_vector();
    std::vector<int> const expect{1, 2, 5, 9};
    EXPECT_TRUE(qb::linq::from(sorted).sequence_equal(expect));
}

/// Right-hand side is a filtered \c enumerable, not a \c vector.
TEST(ChainLinq_Zip, RightOperandIsFilteredPipeline)
{
    std::vector<int> const left{1, 2, 3};
    std::vector<int> const right{10, 20, 30, 40};
    auto rhs = qb::linq::from(right).where(+[](int x) { return x <= 25; });
    std::vector<std::pair<int, int>> pairs;
    for (auto const& pr : qb::linq::from(left).zip(rhs))
        pairs.emplace_back(pr.first, pr.second);
    ASSERT_EQ(pairs.size(), 2u);
    EXPECT_EQ(pairs[0].second, 10);
    EXPECT_EQ(pairs[1].second, 20);
}

TEST(ChainLinq_Zip, InnerWrappedWithAsEnumerable)
{
    std::vector<int> const xs{1, 2};
    std::vector<int> const ys{7, 8};
    auto inner = qb::linq::as_enumerable(qb::linq::from(ys).select(+[](int y) { return y + 1; }));
    int s = 0;
    for (auto const& pr : qb::linq::from(xs).zip(inner))
        s += pr.first + pr.second;
    // (1,8)+(2,9) = 20
    EXPECT_EQ(s, 20);
}

TEST(ChainLinq_Concat, SecondSequenceIsLazyPipeline)
{
    std::vector<int> const a{1, 2};
    std::vector<int> const b{3, 4, 5, 6};
    auto tail = qb::linq::from(b).where(+[](int x) { return x % 2 == 1; });
    auto out = qb::linq::from(a).concat(tail).to_vector();
    std::vector<int> const expect{1, 2, 3, 5};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
}

TEST(ChainLinq_SequenceEqual, TwoIndependentPipelinesSameContent)
{
    std::vector<int> const data{1, 2, 3, 4};
    auto p = qb::linq::from(data).where([](int x) { return x % 2 != 0; });
    auto q = qb::linq::from(data).select([](int x) { return x; }).where([](int x) { return x % 2 != 0; });
    EXPECT_TRUE(p.sequence_equal(q));
}

TEST(ChainLinq_Except, BanListIsLazyProjection)
{
    std::vector<int> const main{1, 2, 3, 4, 5};
    std::vector<int> const raw{2, 4};
    auto ban_view = qb::linq::from(raw).select([](int x) { return x; });
    auto out = qb::linq::from(main).except(ban_view).to_vector();
    std::vector<int> const expect{1, 3, 5};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
}

TEST(ChainLinq_Intersect, WantSetIsFilteredPipeline)
{
    std::vector<int> const left{1, 2, 3, 4};
    std::vector<int> const right{0, 2, 3, 9};
    auto want = qb::linq::from(right).where(+[](int x) { return x > 0 && x < 9; });
    auto out = qb::linq::from(left).intersect(want).to_vector();
    std::vector<int> const expect{2, 3};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
}

TEST(ChainLinq_UnionWith, RhsIsTakeSlice)
{
    std::vector<int> const a{1, 2, 3};
    std::vector<int> const b{3, 4, 5, 6};
    auto rhs = qb::linq::from(b).skip(1).take(2);
    auto out = qb::linq::from(a).union_with(rhs).to_vector();
    std::vector<int> const expect{1, 2, 3, 4, 5};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
}

struct row_o {
    int k;
    int tag;
};

struct row_i {
    int k;
    int v;
};

TEST(ChainLinq_Join, InnerSequenceIsWhereView)
{
    std::vector<row_o> const outer{{1, 100}};
    std::vector<row_i> const inner{{1, 5}, {1, 50}, {2, 9}};
    auto inner_pipe = qb::linq::from(inner).where(+[](row_i const& r) { return r.v < 30; });
    auto j = qb::linq::from(outer).join(inner_pipe,
        [](row_o const& o) { return o.k; },
        [](row_i const& i) { return i.k; },
        [](row_o const& o, row_i const& i) { return o.tag + i.v; });
    EXPECT_EQ(j.single(), 105);
}

TEST(ChainLinq_MaterializedThenLazy, FilterAfterToVector)
{
    std::vector<int> const data{1, 2, 3, 4, 5, 6};
    auto boxed = qb::linq::from(data).where([](int x) { return x % 2 == 0; }).to_vector();
    int const s = boxed.where([](int x) { return x > 2; }).sum();
    EXPECT_EQ(s, 4 + 6);
}

TEST(ChainLinq_GroupJoin, InnerPipeFiltersMatchesPerOuterKey)
{
    std::vector<row_o> const outer{{1, 0}, {2, 0}};
    std::vector<row_i> const inner{{1, 5}, {1, 50}, {2, 1}, {2, 100}};
    auto in_pipe = qb::linq::from(inner).where(+[](row_i const& r) { return r.v >= 10; });
    auto gj = qb::linq::from(outer).group_join(in_pipe,
        [](row_o const& o) { return o.k; },
        [](row_i const& i) { return i.k; });
    ASSERT_EQ(gj.long_count(), 2u);
    int sum_v = 0;
    for (auto const& pr : gj) {
        for (row_i const& r : pr.second)
            sum_v += r.v;
    }
    EXPECT_EQ(sum_v, 50 + 100);
}

TEST(ChainLinq_Join, OuterSequenceIsFilteredPipeline)
{
    std::vector<row_o> const outer{{1, 10}, {2, 20}, {3, 30}};
    std::vector<row_i> const inner{{1, 1}, {3, 1}};
    auto lhs = qb::linq::from(outer).where(+[](row_o const& o) { return o.k != 2; });
    auto j = lhs.join(inner,
        [](row_o const& o) { return o.k; },
        [](row_i const& i) { return i.k; },
        [](row_o const& o, row_i const& i) { return o.tag * i.v; });
    EXPECT_EQ(j.sum(), 10 + 30);
}

TEST(ChainLinq_FactoryIota, ZipsWithContainer)
{
    std::vector<int> const v{100, 200, 300};
    int s = 0;
    for (auto const& pr : qb::linq::iota(0, 3).zip(v))
        s += pr.first + pr.second;
    EXPECT_EQ(s, 100 + 201 + 302);
}

TEST(ChainLinq_FactoryIota, ConcatWithSelectPipeline)
{
    std::vector<int> const tail{8, 9};
    auto rhs = qb::linq::from(tail).select(+[](int x) { return x * 10; });
    auto out = qb::linq::iota(0, 2).concat(rhs).to_vector();
    std::vector<int> const expect{0, 1, 80, 90};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
}

TEST(ChainLinq_DefaultIfEmpty, AfterWhereYieldsSentinel)
{
    std::vector<int> const data{1, 3, 5};
    EXPECT_EQ(qb::linq::from(data).where(+[](int x) { return x % 2 == 0; }).default_if_empty(-7).single(), -7);
}

TEST(ChainLinq_SkipWhileTakeWhile, ChainedPrefixSlice)
{
    std::vector<int> const data{0, 0, 2, 4, 5, 6};
    int const s = qb::linq::from(data)
                      .skip_while(+[](int x) { return x < 2; })
                      .take_while(+[](int x) { return x % 2 == 0; })
                      .sum();
    EXPECT_EQ(s, 6);
}
