/// @file linq_predicates_robustness_test.cpp
/// @brief Broad coverage: predicates, key/projections, comparators, equality hooks, empty/exception paths.

#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <qb/linq.h>

namespace {

// --- Functors with distinct signatures ---

struct pred_by_value {
    [[nodiscard]] bool operator()(int x) const { return x > 0; }
};

struct pred_by_cref {
    [[nodiscard]] bool operator()(int const& x) const { return x % 2 == 0; }
};

struct proj_double {
    [[nodiscard]] double operator()(int const& x) const { return static_cast<double>(x) * 1.5; }
};

struct template_pred {
    template <class T>
    [[nodiscard]] bool operator()(T const& x) const
    {
        return x > 0;
    }
};

struct key_abs_int {
    [[nodiscard]] int operator()(int const& x) const { return x < 0 ? -x : x; }
};

struct key_tens_digit {
    template <class T>
    [[nodiscard]] int operator()(T const& x) const
    {
        return static_cast<int>(x) / 10;
    }
};

struct row_kv {
    int k;
    int v;
};

struct join_row_o {
    int id;
    int payload;
};

struct join_row_i {
    int fk;
    int tag;
};

} // namespace

// ---------------------------------------------------------------------------
// where — callable shapes
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, WhereFunctionPointer)
{
    std::vector<int> const v{-1, 0, 2, 3};
    int const s = qb::linq::from(v).where(+[](int x) { return x > 0; }).sum();
    EXPECT_EQ(s, 5);
}

TEST(PredicatesRobustness, WhereFunctorByValue)
{
    std::vector<int> const v{-2, 3, 4};
    auto const q = qb::linq::from(v).where(pred_by_value{});
    EXPECT_EQ(q.sum(), 7);
}

TEST(PredicatesRobustness, WhereFunctorByConstRefArg)
{
    std::vector<int> const v{1, 2, 3, 4, 5};
    EXPECT_EQ(qb::linq::from(v).where(pred_by_cref{}).sum(), 2 + 4);
}

TEST(PredicatesRobustness, WhereGenericLambda)
{
    std::vector<int> const v{0, 1, -1, 5};
    EXPECT_EQ(qb::linq::from(v).where([](auto const& x) { return x > 0; }).sum(), 6);
}

TEST(PredicatesRobustness, WhereLambdaCapturingConstRef)
{
    int const threshold = 3;
    std::vector<int> const v{1, 2, 3, 4, 5};
    EXPECT_EQ(qb::linq::from(v).where([&threshold](int x) { return x >= threshold; }).sum(), 12);
}

TEST(PredicatesRobustness, WhereMutableCaptureCountsInvocations)
{
    std::vector<int> const v{1, 2, 3};
    int calls = 0;
    auto q = qb::linq::from(v).where([&calls](int) {
        ++calls;
        return true;
    });
    EXPECT_EQ(q.sum(), 6);
    EXPECT_EQ(calls, 3);
}

TEST(PredicatesRobustness, WhereTemplateFunctor)
{
    std::vector<int> const v{-1, 2, 3};
    EXPECT_EQ(qb::linq::from(v).where(template_pred{}).sum(), 5);
}

TEST(PredicatesRobustness, WhereOnConstExprEnumeratedPipeline)
{
    std::vector<int> const v{10, 20, 30};
    auto const pipeline = qb::linq::from(v).where([](int x) { return x >= 20; });
    EXPECT_EQ(pipeline.sum(), 50);
}

// ---------------------------------------------------------------------------
// select — projections
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, SelectFunctorConstRefToDouble)
{
    std::vector<int> const v{2, 4};
    auto vec = qb::linq::from(v).select(proj_double{}).to_vector();
    ASSERT_EQ(vec.long_count(), 2u);
    EXPECT_DOUBLE_EQ(vec.element_at(0), 3.0);
    EXPECT_DOUBLE_EQ(vec.element_at(1), 6.0);
}

TEST(PredicatesRobustness, SelectGenericLambda)
{
    std::vector<int> const v{1, 2, 3};
    auto vec = qb::linq::from(v).select([](auto x) { return x * 10; }).to_vector();
    EXPECT_TRUE(qb::linq::from(vec).sequence_equal(std::vector<int>{10, 20, 30}));
}

TEST(PredicatesRobustness, SelectChainedWhereDifferentCallables)
{
    std::vector<int> const v{-2, 4, 5, -1, 8};
    int const s = qb::linq::from(v)
                      .where(+[](int x) { return x > 0; })
                      .select([](int const& x) { return x * 2; })
                      .where(pred_by_cref{})
                      .sum();
    // 4,5,8 -> *2 -> 8,10,16 -> even -> 8+10+16 = 34
    EXPECT_EQ(s, 34);
}

TEST(PredicatesRobustness, SelectMemberProjection)
{
    struct row {
        int k;
        std::string s;
    };
    std::vector<row> const data{{1, "a"}, {2, "bb"}};
    auto keys = qb::linq::from(data).select([](row const& r) { return r.k; }).to_vector();
    EXPECT_TRUE(qb::linq::from(keys).sequence_equal(std::vector<int>{1, 2}));
}

// ---------------------------------------------------------------------------
// take_while / skip_while style paths
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, TakeWhileConstRefParameter)
{
    std::vector<int> const v{1, 2, 3, 10, 11};
    auto vec = qb::linq::from(v).take_while([](int const& x) { return x < 10; }).to_vector();
    EXPECT_TRUE(qb::linq::from(vec).sequence_equal(std::vector<int>{1, 2, 3}));
}

TEST(PredicatesRobustness, SkipWhileLambdaConstRef)
{
    std::vector<int> const v{-1, -2, 0, 5, 6};
    auto vec = qb::linq::from(v).skip_while([](int const& x) { return x <= 0; }).to_vector();
    EXPECT_TRUE(qb::linq::from(vec).sequence_equal(std::vector<int>{5, 6}));
}

// ---------------------------------------------------------------------------
// zip / concat — inner pipeline with copy-friendly callable (documented caveat)
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, ZipWithInnerWhereUsingFunctionPointer)
{
    std::vector<int> const left{1, 2, 3};
    std::vector<int> const right{-1, 2, 4};
    auto inner = qb::linq::from(right).where(+[](int x) { return x > 0; });
    std::vector<int> sums;
    for (auto const& pr : qb::linq::from(left).zip(inner))
        sums.push_back(pr.first + pr.second);
    ASSERT_EQ(sums.size(), 2u);
    EXPECT_EQ(sums[0], 1 + 2);
    EXPECT_EQ(sums[1], 2 + 4);
}

TEST(PredicatesRobustness, ConcatSecondRangeWithInnerWhereFunctionPointer)
{
    std::vector<int> const first{1, 2};
    std::vector<int> const second{-5, 10, 20};
    auto tail = qb::linq::from(second).where(+[](int x) { return x > 0; });
    auto vec = qb::linq::from(first).concat(tail).to_vector();
    EXPECT_TRUE(qb::linq::from(vec).sequence_equal(std::vector<int>{1, 2, 10, 20}));
}

TEST(PredicatesRobustness, ReverseThenWhereYieldsReverseTraversalOrder)
{
    std::vector<int> const v{1, 2, 3, 4, 5};
    // Traversal: 5,4,3,2,1 — keep even → 4, 2 (order matches walking the reversed sequence).
    auto vec = qb::linq::from(v).reverse().where(pred_by_cref{}).to_vector();
    EXPECT_TRUE(qb::linq::from(vec).sequence_equal(std::vector<int>{4, 2}));
}

// ---------------------------------------------------------------------------
// Empty ranges & vacuous quantifiers
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, EmptyWhereTakeWhileSkipWhileMaterializeToNothing)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).where(pred_by_value{}).sum(), 0);
    EXPECT_EQ(qb::linq::from(empty).take_while(+[](int) { return true; }).long_count(), 0u);
    EXPECT_EQ(qb::linq::from(empty).skip_while(+[](int) { return false; }).long_count(), 0u);
}

TEST(PredicatesRobustness, EmptyAllIfTrueAnyIfFalseNoneIfTrue)
{
    std::vector<int> const empty;
    EXPECT_TRUE(qb::linq::from(empty).all_if(+[](int) { return false; }));
    EXPECT_FALSE(qb::linq::from(empty).any_if(+[](int) { return true; }));
    EXPECT_TRUE(qb::linq::from(empty).none_if(+[](int) { return true; }));
}

TEST(PredicatesRobustness, EmptyCountIfAndSumIf)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).count_if(template_pred{}), 0u);
    EXPECT_EQ(qb::linq::from(empty).long_count_if(pred_by_cref{}), 0u);
    EXPECT_EQ(qb::linq::from(empty).sum_if(pred_by_value{}), 0);
}

TEST(PredicatesRobustness, EmptyFirstOrDefaultIfAndLastOrDefaultIf)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).first_or_default_if(pred_by_value{}), 0);
    EXPECT_EQ(qb::linq::from(empty).last_or_default_if(pred_by_cref{}), 0);
}

TEST(PredicatesRobustness, EmptySingleOrDefaultIf)
{
    std::vector<int> const empty;
    EXPECT_EQ(qb::linq::from(empty).single_or_default_if(+[](int) { return true; }), 0);
}

TEST(PredicatesRobustness, EmptyAverageIfThrows)
{
    std::vector<int> const empty;
    EXPECT_THROW(
        (void)qb::linq::from(empty).average_if(pred_by_value{}), std::out_of_range);
}

TEST(PredicatesRobustness, EmptyFirstIfLastIfSingleIfThrow)
{
    std::vector<int> const empty;
    EXPECT_THROW((void)qb::linq::from(empty).first_if(+[](int) { return true; }), std::out_of_range);
    EXPECT_THROW((void)qb::linq::from(empty).last_if(pred_by_cref{}), std::out_of_range);
    EXPECT_THROW((void)qb::linq::from(empty).single_if(+[](int) { return true; }), std::out_of_range);
}

TEST(PredicatesRobustness, MinByOrDefaultMaxByOrDefaultWithKeyFunctor)
{
    std::vector<int> const empty;
    auto const key = [](int const& x) { return x; };
    EXPECT_EQ(qb::linq::from(empty).min_by_or_default(key), 0);
    EXPECT_EQ(qb::linq::from(empty).max_by_or_default(key), 0);
}

// ---------------------------------------------------------------------------
// first_if / last_if / single_* — non-empty
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, FirstIfAndLastIfReturnExpected)
{
    std::vector<int> const v{3, 4, 5, 6};
    EXPECT_EQ(qb::linq::from(v).first_if(pred_by_cref{}), 4);
    EXPECT_EQ(qb::linq::from(v).last_if(pred_by_cref{}), 6);
}

TEST(PredicatesRobustness, FirstOrDefaultIfLastOrDefaultIf)
{
    std::vector<int> const v{1, 3, 5};
    EXPECT_EQ(qb::linq::from(v).first_or_default_if(pred_by_cref{}), 0);
    EXPECT_EQ(qb::linq::from(v).last_or_default_if(pred_by_cref{}), 0);
    std::vector<int> const w{2, 3, 4};
    EXPECT_EQ(qb::linq::from(w).first_or_default_if(pred_by_cref{}), 2);
    EXPECT_EQ(qb::linq::from(w).last_or_default_if(pred_by_cref{}), 4);
}

TEST(PredicatesRobustness, SingleIfExactlyOneMatch)
{
    std::vector<int> const v{1, 2, 3};
    EXPECT_EQ(qb::linq::from(v).single_if(+[](int x) { return x == 2; }), 2);
}

TEST(PredicatesRobustness, SingleIfNoMatchThrows)
{
    std::vector<int> const v{1, 2, 3};
    EXPECT_THROW((void)qb::linq::from(v).single_if(+[](int x) { return x > 10; }), std::out_of_range);
}

TEST(PredicatesRobustness, SingleIfMoreThanOneMatchThrows)
{
    std::vector<int> const v{2, 4, 6};
    EXPECT_THROW((void)qb::linq::from(v).single_if(pred_by_cref{}), std::out_of_range);
}

TEST(PredicatesRobustness, SingleOrDefaultIfAmbiguousReturnsDefault)
{
    std::vector<int> const v{2, 4, 6};
    EXPECT_EQ(qb::linq::from(v).single_or_default_if(pred_by_cref{}), 0);
}

// ---------------------------------------------------------------------------
// sum_if / average_if / count_if / quantifiers
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, SumIfAverageIfCountIf)
{
    std::vector<int> const v{-2, 3, 4, 5};
    EXPECT_EQ(qb::linq::from(v).sum_if(pred_by_value{}), 12);
    // Even elements: -2 and 4
    EXPECT_DOUBLE_EQ(qb::linq::from(v).average_if(pred_by_cref{}), (-2.0 + 4.0) / 2.0);
    EXPECT_EQ(qb::linq::from(v).count_if(pred_by_value{}), 3u);
    EXPECT_TRUE(qb::linq::from(v).any_if(pred_by_value{}));
    EXPECT_FALSE(qb::linq::from(v).all_if(pred_by_value{}));
    EXPECT_FALSE(qb::linq::from(v).none_if(pred_by_value{}));
}

TEST(PredicatesRobustness, AverageIfNoMatchThrows)
{
    std::vector<int> const v{-1, -2};
    EXPECT_THROW((void)qb::linq::from(v).average_if(pred_by_value{}), std::out_of_range);
}

// ---------------------------------------------------------------------------
// min / max / min_max with custom comparator
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, MinMaxMinMaxWithComparator)
{
    std::vector<int> const v{10, 3, 7};
    auto const less = +[](int a, int b) { return a < b; };
    EXPECT_EQ(qb::linq::from(v).min(less), 3);
    EXPECT_EQ(qb::linq::from(v).max(less), 10);
    auto const mm = qb::linq::from(v).min_max(less);
    EXPECT_EQ(mm.first, 3);
    EXPECT_EQ(mm.second, 10);
}

// ---------------------------------------------------------------------------
// min_by / max_by — key selectors
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, MinByMaxByGenericLambdaKey)
{
    std::vector<std::pair<int, int>> const v{{5, 100}, {2, 200}, {8, 50}};
    auto const key_first = [](auto const& p) { return p.first; };
    auto const key_second = [](auto const& p) { return p.second; };
    EXPECT_EQ(qb::linq::from(v).min_by(key_first).second, 200);
    EXPECT_EQ(qb::linq::from(v).max_by(key_second).first, 2);
}

// ---------------------------------------------------------------------------
// distinct_by (lazy key function)
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, DistinctByFunctorAndFunctionPointer)
{
    std::vector<int> const v{1, -1, 2, -2, 3};
    auto a = qb::linq::from(v).distinct_by(key_abs_int{}).to_vector();
    EXPECT_EQ(a.long_count(), 3u);
    auto b = qb::linq::from(v).distinct_by(+[](int const& x) { return x < 0 ? -x : x; }).to_vector();
    EXPECT_TRUE(qb::linq::from(a).sequence_equal(b));
}

// ---------------------------------------------------------------------------
// order_by / group_by / to_lookup — key callables
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, OrderByAscDescConstRefKey)
{
    std::vector<int> const data{3, 1, 4};
    auto up = qb::linq::from(data).order_by(qb::linq::asc([](int const& x) { return x; })).to_vector();
    EXPECT_TRUE(qb::linq::from(up).sequence_equal(std::vector<int>{1, 3, 4}));
    auto down = qb::linq::from(data).order_by(qb::linq::desc([](int const& x) { return x; })).to_vector();
    EXPECT_TRUE(qb::linq::from(down).sequence_equal(std::vector<int>{4, 3, 1}));
}

TEST(PredicatesRobustness, GroupByTemplateFunctorTensDigit)
{
    std::vector<int> const data{10, 21, 32};
    auto g = qb::linq::from(data).group_by(key_tens_digit{});
    EXPECT_EQ(qb::linq::from(g[1]).sum(), 10);
    EXPECT_EQ(qb::linq::from(g[2]).sum(), 21);
    EXPECT_EQ(qb::linq::from(g[3]).sum(), 32);
}

TEST(PredicatesRobustness, ToLookupSameAsGroupByWithFunctionPointer)
{
    std::vector<int> const data{5, 15, 25};
    auto a = qb::linq::from(data).group_by(+[](int const& x) { return x / 10; });
    auto b = qb::linq::from(data).to_lookup(+[](int const& x) { return x / 10; });
    EXPECT_EQ(qb::linq::from(a[0]).sum(), qb::linq::from(b[0]).sum());
    EXPECT_EQ(qb::linq::from(a[1]).sum(), qb::linq::from(b[1]).sum());
    EXPECT_EQ(qb::linq::from(a[2]).sum(), qb::linq::from(b[2]).sum());
}

// ---------------------------------------------------------------------------
// join / group_join — keys + result selector
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, JoinAllFunctionPointers)
{
    std::vector<join_row_o> const outer{{1, 100}, {2, 200}};
    std::vector<join_row_i> const inner{{1, 1}, {1, 2}, {2, 3}};
    auto j = qb::linq::from(outer)
                 .join(inner,
                     +[](join_row_o const& o) { return o.id; },
                     +[](join_row_i const& i) { return i.fk; },
                     +[](join_row_o const& o, join_row_i const& i) {
                         return std::make_pair(o.payload, i.tag);
                     });
    int sum_payload = 0;
    int sum_tag = 0;
    for (auto const& p : j) {
        sum_payload += p.first;
        sum_tag += p.second;
    }
    EXPECT_EQ(sum_payload, 100 + 100 + 200);
    EXPECT_EQ(sum_tag, 1 + 2 + 3);
}

TEST(PredicatesRobustness, GroupJoinFunctorKeys)
{
    std::vector<join_row_o> const outer{{7, 0}};
    std::vector<join_row_i> const inner{{7, 10}, {7, 20}};
    auto gj = qb::linq::from(outer).group_join(inner,
        [](join_row_o const& o) { return o.id; },
        [](join_row_i const& i) { return i.fk; });
    ASSERT_EQ(gj.long_count(), 1u);
    auto const& row = *gj.begin();
    EXPECT_EQ(row.second.size(), 2u);
    EXPECT_EQ(row.second[0].tag + row.second[1].tag, 30);
}

// ---------------------------------------------------------------------------
// contains / index_of / last_index_of / sequence_equal — custom Eq / Comp
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, ContainsIndexOfWithEq)
{
    std::vector<int> const v{1, 2, 3};
    auto const mod_eq = +[](int a, int b) { return (a % 2) == (b % 2); };
    EXPECT_TRUE(qb::linq::from(v).contains(9, mod_eq));
    EXPECT_EQ(qb::linq::from(v).index_of(4, mod_eq).value_or(999u), 1u); // first even at value 2
    EXPECT_EQ(qb::linq::from(v).last_index_of(6, mod_eq).value_or(999u), 1u);
}

TEST(PredicatesRobustness, SequenceEqualCustomComparator)
{
    std::vector<int> const a{1, 2, 3};
    std::vector<int> const b{-1, -2, -3};
    auto const same_sq = +[](int x, int y) { return x * x == y * y; };
    EXPECT_TRUE(qb::linq::from(a).sequence_equal(b, same_sq));
}

// ---------------------------------------------------------------------------
// each / aggregate / reduce / zip_fold / scan — callables on elements
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, EachTakesConstRefToElement)
{
    std::vector<int> const v{1, 2};
    int s = 0;
    qb::linq::from(v).each([&s](int const& x) { s += x; });
    EXPECT_EQ(s, 3);
}

TEST(PredicatesRobustness, AggregateReduceGenericLambda)
{
    std::vector<int> const v{1, 2, 3};
    EXPECT_EQ(qb::linq::from(v).aggregate(10, [](int a, int b) { return a + b; }), 16);
    EXPECT_EQ(qb::linq::from(v).reduce([](auto acc, auto x) { return acc * 10 + x; }), 123);
}

TEST(PredicatesRobustness, ZipFoldCallable)
{
    std::vector<int> const a{1, 2};
    std::vector<int> const b{10, 20};
    int const z = qb::linq::from(a).zip_fold(b, 0, [](int acc, int const& x, int const& y) {
        return acc + x + y;
    });
    EXPECT_EQ(z, 1 + 10 + 2 + 20);
}

TEST(PredicatesRobustness, ScanWithConstRefInFolder)
{
    std::vector<int> const v{2, 3};
    std::vector<int> out;
    for (int x : qb::linq::from(v).scan(0, [](int acc, int const& cur) { return acc + cur; }))
        out.push_back(x);
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0], 2);
    EXPECT_EQ(out[1], 5);
}

// ---------------------------------------------------------------------------
// to_map / to_unordered_map — key + element selectors
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, ToMapToUnorderedMapFunctors)
{
    std::vector<row_kv> const rows{{3, 30}, {1, 10}};
    auto ord = qb::linq::from(rows).to_map(
        [](row_kv const& r) { return r.k; },
        [](row_kv const& r) { return r.v; });
    EXPECT_EQ(ord[1], 10);
    EXPECT_EQ(ord[3], 30);
    auto uo = qb::linq::from(rows).to_unordered_map(
        +[](row_kv const& r) { return r.k; },
        +[](row_kv const& r) { return r.v; });
    EXPECT_EQ(uo[3], 30);
}

// ---------------------------------------------------------------------------
// concat / union_with — mixed const-qualification on iterator reference (regression)
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, UnionWithMutableVectorAndConstRhsRange)
{
    std::vector<int> a{1, 2, 3};
    std::vector<int> const b{2, 3, 4};
    std::vector<int> out;
    for (int x : qb::linq::from(a).union_with(b))
        out.push_back(x);
    std::sort(out.begin(), out.end());
    std::vector<int> const expect{1, 2, 3, 4};
    EXPECT_EQ(out, expect);
}

/**
 * `take(n)` must not advance the underlying filtered iterator after the n-th yield (no extra `where` calls).
 * Regression: naive `++remaining; ++base` ran predicates on trailing elements (e.g. 100+4) after the last take.
 */
TEST(PredicatesRobustness, TakeShortCircuitsUnderlyingAfterLastYield)
{
    std::vector<int> log;
    std::vector<int> src{1, 2, 3, 4, 5};

    auto q = qb::linq::from(src)
                 .where([&](int x) {
                     log.push_back(100 + x);
                     return x % 2 == 1;
                 })
                 .select([&](int x) {
                     log.push_back(200 + x);
                     return x * 10;
                 })
                 .take(2);

    std::vector<int> out;
    for (int x : q.to_vector())
        out.push_back(x);

    std::vector<int> const expect{10, 30};
    EXPECT_EQ(out, expect);
    EXPECT_EQ(std::find(log.begin(), log.end(), 104), log.end());
    EXPECT_EQ(std::find(log.begin(), log.end(), 105), log.end());
    EXPECT_NE(std::find(log.begin(), log.end(), 201), log.end());
    EXPECT_NE(std::find(log.begin(), log.end(), 203), log.end());
}

/**
 * `from(container)` captures iterators when the pipeline is built; after `vector::push_back`, re-build `from(v)`
 * to see new elements. Holding an old pipeline across invalidating mutation is undefined behavior (C++).
 */
TEST(PredicatesRobustness, PipelineRebuildAfterVectorGrowthSeesNewElements)
{
    std::vector<int> v{1, 2, 3, 4, 5};
    std::vector<int> before;
    for (int x : qb::linq::from(v).where([](int x) { return x % 2 == 0; }).select([](int x) { return x * 10; }))
        before.push_back(x);
    EXPECT_EQ(before, (std::vector<int>{20, 40}));

    v.push_back(6);
    std::vector<int> after;
    for (int x : qb::linq::from(v).where([](int x) { return x % 2 == 0; }).select([](int x) { return x * 10; }))
        after.push_back(x);
    EXPECT_EQ(after, (std::vector<int>{20, 40, 60}));
}

/** Raw `concat` (not only `union_with`) must accept `T&` / `T const&` legs like `concat_iterator::reference`. */
TEST(PredicatesRobustness, ConcatMutableVectorWithConstRhsRange)
{
    std::vector<int> a{7, 8};
    std::vector<int> const b{9};
    std::vector<int> out;
    for (int x : qb::linq::from(a).concat(b))
        out.push_back(x);
    EXPECT_EQ(out, (std::vector<int>{7, 8, 9}));
}

/** `zip` pairs `iterator_traits<It1>::reference` with `It2::reference` — mixed const on the RHS must compile. */
TEST(PredicatesRobustness, ZipMutableLeftConstRightRange)
{
    std::vector<int> a{1, 2};
    std::vector<int> const b{30, 40};
    std::vector<int> sums;
    for (auto const& pr : qb::linq::from(a).zip(b))
        sums.push_back(pr.first + pr.second);
    EXPECT_EQ(sums, (std::vector<int>{31, 42}));
}

/** `take(1)` on a filtered range must not invoke `where` past the single yielded element. */
TEST(PredicatesRobustness, TakeOneStopsWhereWithoutWalkingTail)
{
    std::vector<int> log;
    std::vector<int> const src{1, 2, 3};
    for (int x : qb::linq::from(src)
                     .where([&](int v) {
                         log.push_back(100 + v);
                         return true;
                     })
                     .take(1)
                     .to_vector())
        (void)x;
    ASSERT_EQ(log.size(), 1u);
    EXPECT_EQ(log[0], 101);
}

/** `skip` then `take` on logged `where`: no predicate on elements after the taken yield. */
TEST(PredicatesRobustness, SkipThenTakeShortCircuitsWhereTail)
{
    std::vector<int> log;
    std::vector<int> const src{1, 2, 3, 4, 5};
    for (int x : qb::linq::from(src)
                     .skip(2)
                     .where([&](int v) {
                         log.push_back(v);
                         return true;
                     })
                     .take(1)
                     .to_vector())
        (void)x;
    ASSERT_EQ(log.size(), 1u);
    EXPECT_EQ(log[0], 3);
}

/** Bidirectional composition: `reverse` after `take` must still see exactly the taken prefix, reversed. */
TEST(PredicatesRobustness, ReverseAfterTakeYieldsPrefixReversed)
{
    std::vector<int> const v{1, 2, 3, 4, 5};
    std::vector<int> out;
    for (int x : qb::linq::from(v).take(3).reverse().to_vector())
        out.push_back(x);
    EXPECT_EQ(out, (std::vector<int>{3, 2, 1}));
}

/** `take_while` also uses a logical end paired with physical `end()`; reverse must stay inside the prefix. */
TEST(PredicatesRobustness, ReverseAfterTakeWhileYieldsAcceptedPrefixReversed)
{
    std::vector<int> const v{1, 2, 3, 4, 5};
    std::vector<int> out;
    for (int x : qb::linq::from(v).take_while([](int x) { return x < 4; }).reverse().to_vector())
        out.push_back(x);
    EXPECT_EQ(out, (std::vector<int>{3, 2, 1}));
}

/** Exhaustion at physical `end()` (`exh.base() == sen.base()`): still full reversal on underlying bases. */
TEST(PredicatesRobustness, ReverseAfterTakeWhile_AllElementsAcceptedThroughEnd)
{
    std::vector<int> const v{1, 2, 3};
    std::vector<int> out;
    for (int x : qb::linq::from(v).take_while([](int x) { return x < 100; }).reverse().to_vector())
        out.push_back(x);
    EXPECT_EQ(out, (std::vector<int>{3, 2, 1}));
}

/** First element fails: forward range empty; reversed range must stay empty (`!any_step` in specialization). */
TEST(PredicatesRobustness, ReverseAfterTakeWhile_NoAcceptedElements)
{
    std::vector<int> const v{5, 6, 1};
    EXPECT_EQ(qb::linq::from(v).take_while([](int x) { return x < 2; }).reverse().long_count(), 0u);
}

/** Single accepted element then stop: reverse is trivially that element. */
TEST(PredicatesRobustness, ReverseAfterTakeWhile_SingleElementPrefix)
{
    std::vector<int> const v{1, 9, 2};
    std::vector<int> out;
    for (int x : qb::linq::from(v).take_while([](int x) { return x < 5; }).reverse().to_vector())
        out.push_back(x);
    EXPECT_EQ(out, (std::vector<int>{1}));
}

/** `take(0)`: empty take window; reverse must not resurrect elements. */
TEST(PredicatesRobustness, ReverseAfterTake_ZeroCountIsEmpty)
{
    std::vector<int> const v{1, 2, 3};
    EXPECT_EQ(qb::linq::from(v).take(0).reverse().long_count(), 0u);
}

/** `take(n)` with `n` larger than length: reverse is full sequence reversed. */
TEST(PredicatesRobustness, ReverseAfterTake_CountExceedsSourceLength)
{
    std::vector<int> const v{7, 8};
    std::vector<int> out;
    for (int x : qb::linq::from(v).take(1000).reverse().to_vector())
        out.push_back(x);
    EXPECT_EQ(out, (std::vector<int>{8, 7}));
}

/** Composition: `skip` narrows bases before `take_while` scan + base-only reverse. */
TEST(PredicatesRobustness, ReverseAfterSkipAndTakeWhile)
{
    std::vector<int> const v{0, 1, 2, 3, 4};
    std::vector<int> out;
    for (int x : qb::linq::from(v).skip(1).take_while([](int x) { return x < 3; }).reverse().to_vector())
        out.push_back(x);
    EXPECT_EQ(out, (std::vector<int>{2, 1}));
}

// ---------------------------------------------------------------------------
// Statistics — key functions
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, PercentileByAndVarianceByConstRefKey)
{
    std::vector<int> const v{0, 10, 20};
    double const p50 = qb::linq::from(v).percentile_by(50.0, [](int const& x) { return static_cast<double>(x); });
    EXPECT_NEAR(p50, 10.0, 1e-9);
    double const var = qb::linq::from(v).variance_population_by([](int const& x) { return static_cast<double>(x); });
    EXPECT_GT(var, 0.0);
    double const sd = qb::linq::from(v).std_dev_sample_by([](int const& x) { return static_cast<double>(x); });
    EXPECT_GT(sd, 0.0);
}

// ---------------------------------------------------------------------------
// select_many — multiple projections per row
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, SelectManyMixedLambdas)
{
    std::vector<row_kv> const data{{1, 10}, {2, 20}};
    auto q = qb::linq::from(data).select_many(
        [](row_kv const& r) { return r.k; },
        [](row_kv const& r) { return r.v * 2; });
    std::vector<std::tuple<int, int>> got;
    for (auto const& t : q)
        got.push_back(t);
    ASSERT_EQ(got.size(), 2u);
    EXPECT_EQ(std::get<0>(got[0]), 1);
    EXPECT_EQ(std::get<1>(got[0]), 20);
}

// ---------------------------------------------------------------------------
// where matches nothing / take_while full range
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, WhereNoMatches)
{
    std::vector<int> const v{1, 2, 3};
    EXPECT_FALSE(qb::linq::from(v).where(+[](int x) { return x < 0; }).any());
}

TEST(PredicatesRobustness, TakeWhileEntireSequence)
{
    std::vector<int> const v{1, 2};
    EXPECT_EQ(qb::linq::from(v).take_while(+[](int const& x) { return x < 10; }).sum(), 3);
}

// ---------------------------------------------------------------------------
// reverse() composed with other views — regression for non–default-constructible
// base iterators (e.g. libstdc++ + basic_iterator); see where_view cache design.
// ---------------------------------------------------------------------------

TEST(PredicatesRobustness, ReverseThenDistinctBy)
{
    std::vector<int> const v{1, 2, 3, 1};
    int const s = qb::linq::from(v).reverse().distinct_by(+[](int const& x) { return x; }).sum();
    EXPECT_EQ(s, 1 + 3 + 2);
}

TEST(PredicatesRobustness, ReverseThenZip)
{
    std::vector<int> const v{1, 2};
    std::vector<int> const w{10, 20};
    int acc = 0;
    for (auto const& pr : qb::linq::from(v).reverse().zip(w))
        acc += pr.first + pr.second;
    EXPECT_EQ(acc, (2 + 10) + (1 + 20));
}

TEST(PredicatesRobustness, ReverseThenScan)
{
    std::vector<int> const v{2, 3};
    std::vector<int> out;
    for (int x : qb::linq::from(v).reverse().scan(0, [](int a, int const& b) { return a + b; }))
        out.push_back(x);
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0], 3);
    EXPECT_EQ(out[1], 5);
}

TEST(PredicatesRobustness, ReverseThenConcat)
{
    std::vector<int> const v{1, 2};
    std::vector<int> const tail{7};
    auto vec = qb::linq::from(v).reverse().concat(tail).to_vector();
    EXPECT_TRUE(qb::linq::from(vec).sequence_equal(std::vector<int>{2, 1, 7}));
}