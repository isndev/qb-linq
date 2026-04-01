/// @file linq_robustness_parity_test.cpp
/// @brief Extreme / empty inputs and invariants for outer joins, zip3, *By, count_by, try_get, indexed ops.

#include <list>
#include <random>
#include <string>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

namespace {

struct P {
    int k;
    int v;
};

} // namespace

// --- Outer joins: empty ranges (must not throw; exact counts) ---

TEST(RobustnessOuterJoin, LeftJoinEmptyOuterYieldsNothing)
{
    std::vector<P> const outers{};
    std::vector<P> const inners{{1, 10}};
    auto rows = qb::linq::from(outers)
                    .left_join(inners,
                        [](P const& x) { return x.k; },
                        [](P const& x) { return x.k; },
                        [](P const&, std::optional<P> const& o) { return o->v; });
    EXPECT_EQ(rows.count(), 0u);
}

TEST(RobustnessOuterJoin, LeftJoinEmptyOuterEmptyInner)
{
    std::vector<int> const a{};
    std::vector<int> const b{};
    auto n = qb::linq::from(a)
                 .left_join(b, [](int x) { return x; }, [](int x) { return x; },
                     [](int o, std::optional<int> const& i) { return o + *i; })
                 .count();
    EXPECT_EQ(n, 0u);
}

TEST(RobustnessOuterJoin, RightJoinEmptyInnerYieldsNothing)
{
    std::vector<P> const outers{{1, 1}};
    std::vector<P> const inners{};
    auto rows = qb::linq::from(outers)
                    .right_join(inners,
                        [](P const& x) { return x.k; },
                        [](P const& x) { return x.k; },
                        [](std::optional<P> const&, P const&) { return 0; });
    EXPECT_EQ(rows.count(), 0u);
}

TEST(RobustnessOuterJoin, RightJoinBothEmpty)
{
    std::vector<int> const a{};
    std::vector<int> const b{};
    auto n = qb::linq::from(a)
                 .right_join(b, [](int x) { return x; }, [](int x) { return x; },
                     [](std::optional<int> const&, int) { return 1; })
                 .count();
    EXPECT_EQ(n, 0u);
}

// --- zip3: any empty leg → zero length ---

TEST(RobustnessZip3, FirstLegEmpty)
{
    std::vector<int> a{};
    std::vector<int> b{1};
    std::vector<int> c{2};
    EXPECT_EQ(qb::linq::from(a).zip(b, c).count(), 0u);
}

TEST(RobustnessZip3, MiddleLegEmpty)
{
    std::vector<int> a{1};
    std::vector<int> b{};
    std::vector<int> c{2};
    EXPECT_EQ(qb::linq::from(a).zip(b, c).count(), 0u);
}

TEST(RobustnessZip3, LastLegEmpty)
{
    std::vector<int> a{1};
    std::vector<int> b{2};
    std::vector<int> c{};
    EXPECT_EQ(qb::linq::from(a).zip(b, c).count(), 0u);
}

// --- count_by / *By / indexed on empty ---

TEST(RobustnessCountBy, EmptySource)
{
    std::vector<std::string> const empty{};
    EXPECT_EQ(qb::linq::from(empty).count_by([](std::string const& s) { return s; }).count(), 0u);
}

TEST(RobustnessExceptBy, EmptyLeft)
{
    std::vector<P> const left{};
    std::vector<int> const right{1, 2};
    auto out = qb::linq::from(left)
                   .except_by(right, [](P const& x) { return x.k; }, [](int x) { return x; })
                   .to_vector();
    EXPECT_EQ(out.count(), 0u);
}

TEST(RobustnessIntersectBy, EmptyLeft)
{
    std::vector<P> const left{};
    std::vector<int> const right{1};
    EXPECT_EQ(qb::linq::from(left)
                  .intersect_by(right, [](P const& x) { return x.k; }, [](int x) { return x; })
                  .count(),
        0u);
}

TEST(RobustnessUnionBy, BothEmpty)
{
    std::vector<std::pair<int, char>> const a{};
    std::vector<std::pair<int, char>> const b{};
    EXPECT_EQ(qb::linq::from(a)
                  .union_by(b, [](auto const& p) { return p.first; }, [](auto const& p) { return p.first; })
                  .count(),
        0u);
}

TEST(RobustnessIndexed, SelectWhereOnEmpty)
{
    std::vector<int> const e{};
    EXPECT_EQ(qb::linq::from(e).select_indexed([](int x, std::size_t) { return x; }).count(), 0u);
    EXPECT_EQ(qb::linq::from(e).where_indexed([](int, std::size_t) { return true; }).count(), 0u);
}

// --- try_get_non_enumerated_count: forward-only pipeline → nullopt ---

TEST(RobustnessTryGetNonEnumeratedCount, AfterWhereOnListIsForward)
{
    std::list<int> L{1, 2, 3, 4};
    auto q = qb::linq::from(L).where([](int x) { return x % 2 == 0; });
    EXPECT_FALSE(q.try_get_non_enumerated_count().has_value());
}

TEST(RobustnessTryGetNonEnumeratedCount, IotaIsNotRandomAccess)
{
    auto r = qb::linq::iota(0, 10);
    EXPECT_FALSE(r.try_get_non_enumerated_count().has_value());
}

/** `take_n_iterator` clamps category to bidirectional — no O(1) size without walking. */
TEST(RobustnessTryGetNonEnumeratedCount, AfterTakeOnVectorReturnsNullopt)
{
    std::vector<int> v{1, 2, 3, 4, 5};
    auto q = qb::linq::from(v).take(3);
    EXPECT_FALSE(q.try_get_non_enumerated_count().has_value());
}

/** Materialized `vector` span keeps random-access iterators. */
TEST(RobustnessTryGetNonEnumeratedCount, AfterToVectorHasEnumeratedCount)
{
    std::vector<int> v{1, 2, 3};
    auto m = qb::linq::from(v).to_vector();
    auto const n = m.try_get_non_enumerated_count();
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, 3u);
}

// --- Inner join: empty operand → no rows ---

TEST(RobustnessInnerJoin, EmptyOuterOrInnerYieldsNothing)
{
    std::vector<int> const a{1, 2};
    std::vector<int> const empty{};
    auto j0 = qb::linq::from(empty)
                  .join(a, [](int x) { return x; }, [](int x) { return x; },
                      [](int x, int y) { return x + y; })
                  .count();
    auto j1 = qb::linq::from(a)
                  .join(empty, [](int x) { return x; }, [](int x) { return x; },
                      [](int x, int y) { return x + y; })
                  .count();
    EXPECT_EQ(j0, 0u);
    EXPECT_EQ(j1, 0u);
}

// --- Invariant: left_join row count formula ---

TEST(RobustnessOuterJoin, LeftJoinRowCountEqualsReferenceSum)
{
    std::mt19937 rng(9001);
    std::uniform_int_distribution<int> nk(0, 6);
    for (int t = 0; t < 80; ++t) {
        std::vector<int> keys_o;
        std::vector<int> keys_i;
        int const no = nk(rng);
        int const ni = nk(rng);
        for (int i = 0; i < no; ++i)
            keys_o.push_back(nk(rng));
        for (int i = 0; i < ni; ++i)
            keys_i.push_back(nk(rng));

        std::size_t ref_count = 0;
        for (int ko : keys_o) {
            std::size_t m = 0;
            for (int ki : keys_i) {
                if (ki == ko)
                    ++m;
            }
            ref_count += (m == 0) ? 1 : m;
        }

        auto got = qb::linq::from(keys_o)
                       .left_join(keys_i,
                           [](int x) { return x; },
                           [](int x) { return x; },
                           [](int, std::optional<int> const&) { return 0; })
                       .count();
        EXPECT_EQ(got, ref_count) << "trial " << t;
    }
}

// --- Invariant: right_join row count (per inner row: max(1, #matching outers)) ---

TEST(RobustnessOuterJoin, RightJoinRowCountEqualsReferenceSum)
{
    std::mt19937 rng(4243);
    std::uniform_int_distribution<int> nk(0, 6);
    for (int t = 0; t < 80; ++t) {
        std::vector<int> keys_o;
        std::vector<int> keys_i;
        int const no = nk(rng);
        int const ni = nk(rng);
        for (int i = 0; i < no; ++i)
            keys_o.push_back(nk(rng));
        for (int i = 0; i < ni; ++i)
            keys_i.push_back(nk(rng));

        std::size_t ref_count = 0;
        for (int ki : keys_i) {
            std::size_t m = 0;
            for (int ko : keys_o) {
                if (ko == ki)
                    ++m;
            }
            ref_count += (m == 0) ? 1 : m;
        }

        auto got = qb::linq::from(keys_o)
                       .right_join(keys_i,
                           [](int x) { return x; },
                           [](int x) { return x; },
                           [](std::optional<int> const&, int) { return 0; })
                       .count();
        EXPECT_EQ(got, ref_count) << "trial " << t;
    }
}
