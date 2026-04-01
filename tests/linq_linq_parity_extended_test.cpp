/// @file linq_linq_parity_extended_test.cpp
/// @brief Indexed select/where, three-way zip, key-based set ops, count_by, try_get_non_enumerated_count.

#include <list>
#include <string>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(SelectIndexed, PairsValueWithIndex)
{
    std::vector<int> const v{10, 20, 30};
    auto const got = qb::linq::from(v).select_indexed([](int x, std::size_t i) { return x + static_cast<int>(i); }).to_vector();
    std::vector<int> const expect{10, 21, 32};
    EXPECT_TRUE(qb::linq::from(got).sequence_equal(expect));
}

TEST(WhereIndexed, FiltersByIndex)
{
    std::vector<int> v{1, 2, 3, 4, 5};
    auto const got = qb::linq::from(v).where_indexed([](int, std::size_t i) { return i % 2 == 0; }).to_vector();
    std::vector<int> const expect{1, 3, 5};
    EXPECT_TRUE(qb::linq::from(got).sequence_equal(expect));
}

TEST(ZipThree, ShortestWinsAndTupleRefs)
{
    std::vector<int> a{1, 2, 3};
    std::vector<char> b{'a', 'b'};
    std::vector<double> c{0.1, 0.2, 0.3};
    auto z = qb::linq::from(a).zip(b, c);
    ASSERT_EQ(z.count(), 2u);
    auto it = z.begin();
    {
        auto t = *it;
        EXPECT_EQ(std::get<0>(t), 1);
        EXPECT_EQ(std::get<1>(t), 'a');
        EXPECT_DOUBLE_EQ(std::get<2>(t), 0.1);
    }
    ++it;
    {
        auto t = *it;
        EXPECT_EQ(std::get<0>(t), 2);
        EXPECT_EQ(std::get<1>(t), 'b');
        EXPECT_DOUBLE_EQ(std::get<2>(t), 0.2);
    }
    ++it;
    EXPECT_EQ(it, z.end());
}

TEST(ExceptBy, KeysFromRightBanLeft)
{
    struct L {
        int id;
        char tag;
    };
    struct R {
        int k;
    };
    std::vector<L> const left{{1, 'a'}, {2, 'b'}, {3, 'c'}, {2, 'x'}};
    std::vector<R> const right{{2}, {99}};
    auto out = qb::linq::from(left)
                   .except_by(right, [](L const& x) { return x.id; }, [](R const& y) { return y.k; })
                   .to_vector();
    ASSERT_EQ(out.count(), 2u);
    auto oi = out.begin();
    EXPECT_EQ(oi->id, 1);
    ++oi;
    EXPECT_EQ(oi->id, 3);
}

TEST(IntersectBy, DistinctLeftKeysInWantSet)
{
    struct Row {
        int k;
        std::string s;
    };
    std::vector<Row> const left{{1, "a"}, {2, "b"}, {1, "dup"}, {3, "c"}};
    std::vector<int> const keys{2, 3, 3};
    auto out = qb::linq::from(left)
                   .intersect_by(keys, [](Row const& r) { return r.k; }, [](int x) { return x; })
                   .to_vector();
    ASSERT_EQ(out.count(), 2u);
    auto oi = out.begin();
    EXPECT_EQ(oi->k, 2);
    ++oi;
    EXPECT_EQ(oi->k, 3);
}

TEST(UnionBy, LeftThenRightNewKeys)
{
    std::vector<std::pair<int, char>> const a{{1, 'a'}, {2, 'b'}};
    std::vector<std::pair<int, char>> const b{{2, 'x'}, {3, 'y'}};
    auto out = qb::linq::from(a)
                   .union_by(b, [](auto const& p) { return p.first; }, [](auto const& p) { return p.first; })
                   .to_vector();
    ASSERT_EQ(out.count(), 3u);
    auto oi = out.begin();
    EXPECT_EQ(oi->first, 1);
    ++oi;
    EXPECT_EQ(oi->first, 2);
    ++oi;
    EXPECT_EQ(oi->first, 3);
    EXPECT_EQ(oi->second, 'y');
}

TEST(CountBy, OrderAndCounts)
{
    std::vector<std::string> const words{"a", "b", "a", "c", "b", "a"};
    auto rows = qb::linq::from(words).count_by([](std::string const& s) { return s; });
    ASSERT_EQ(rows.count(), 3u);
    auto ri = rows.begin();
    EXPECT_EQ(ri->first, "a");
    EXPECT_EQ(ri->second, 3);
    ++ri;
    EXPECT_EQ(ri->first, "b");
    EXPECT_EQ(ri->second, 2);
    ++ri;
    EXPECT_EQ(ri->first, "c");
    EXPECT_EQ(ri->second, 1);
}

TEST(TryGetNonEnumeratedCount, VectorHasSizeListDoesNot)
{
    std::vector<int> v{1, 2, 3};
    auto ev = qb::linq::from(v);
    auto const n = ev.try_get_non_enumerated_count();
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, 3u);

    std::list<int> L{1, 2, 3};
    auto el = qb::linq::from(L);
    EXPECT_FALSE(el.try_get_non_enumerated_count().has_value());
}

TEST(WhereIndexed, PreservesReferenceIntoSource)
{
    std::vector<int> v{10, 20, 30};
    int* first_addr = &v[0];
    auto q = qb::linq::from(v).where_indexed([](int, std::size_t i) { return i == 0; });
    auto it = q.begin();
    ASSERT_NE(it, q.end());
    EXPECT_EQ(&*it, first_addr);
}
