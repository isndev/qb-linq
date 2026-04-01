/// @file linq_join_set_test.cpp
/// @brief Relational-style join / group_join and set-style operations (`union_with`, `except`, `intersect`).

#include <algorithm>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

struct Person {
    int id;
    std::string name;
};

struct Score {
    int person_id;
    int points;
};

TEST(JoinInner, MatchesNestedLoop)
{
    std::vector<Person> const people{{1, "Ann"}, {2, "Bob"}, {3, "Cam"}};
    std::vector<Score> const scores{{1, 10}, {1, 11}, {2, 20}, {99, 1}};

    auto joined = qb::linq::from(people)
                      .join(scores,
                          [](Person const& p) { return p.id; },
                          [](Score const& s) { return s.person_id; },
                          [](Person const& p, Score const& s) {
                              return std::make_tuple(p.name, s.points);
                          });

    std::vector<std::tuple<std::string, int>> manual;
    for (Person const& p : people) {
        for (Score const& s : scores) {
            if (p.id == s.person_id)
                manual.emplace_back(p.name, s.points);
        }
    }

    EXPECT_EQ(joined.long_count(), manual.size());
    EXPECT_EQ(joined.long_count(), 3u);

    int sum_pts = 0;
    for (auto const& t : joined)
        sum_pts += std::get<1>(t);
    EXPECT_EQ(sum_pts, 10 + 11 + 20);
}

TEST(GroupJoin, BucketsInnerRows)
{
    std::vector<Person> const people{{1, "Ann"}, {2, "Bob"}};
    std::vector<Score> const scores{{1, 5}, {1, 7}, {2, 3}};

    auto gj = qb::linq::from(people).group_join(scores,
        [](Person const& p) { return p.id; },
        [](Score const& s) { return s.person_id; });

    ASSERT_EQ(gj.long_count(), 2u);
    for (auto const& row : gj) {
        if (row.first.id == 1) {
            ASSERT_EQ(row.second.size(), 2u);
            EXPECT_EQ(row.second[0].points + row.second[1].points, 5 + 7);
        } else if (row.first.id == 2) {
            ASSERT_EQ(row.second.size(), 1u);
            EXPECT_EQ(row.second[0].points, 3);
        }
    }
}

TEST(UnionWith, ConcatThenDistinct)
{
    std::vector<int> const a{1, 2, 3};
    std::vector<int> const b{3, 4};
    auto u = qb::linq::from(a).union_with(b).to_vector();
    std::vector<int> const expect{1, 2, 3, 4};
    EXPECT_TRUE(qb::linq::from(u).sequence_equal(expect));
}

/** `union_with(Rng const&)` uses const iterators on the RHS; `concat_iterator` must use a common reference type. */
TEST(UnionWith, MutableLeftConstRightCompilesAndDedupes)
{
    std::vector<int> a{1, 2, 3, 4};
    std::vector<int> const b{3, 4, 5};
    std::vector<int> collected;
    for (int x : qb::linq::from(a).union_with(b))
        collected.push_back(x);
    std::sort(collected.begin(), collected.end());
    std::vector<int> const expect{1, 2, 3, 4, 5};
    EXPECT_EQ(collected, expect);
}

TEST(JoinInner, NoMatchesYieldsEmpty)
{
    std::vector<Person> const people{{1, "Ann"}};
    std::vector<Score> const scores{{99, 1}, {98, 2}};
    auto joined = qb::linq::from(people)
                      .join(scores,
                          [](Person const& p) { return p.id; },
                          [](Score const& s) { return s.person_id; },
                          [](Person const& p, Score const& s) { return std::make_pair(p.name, s.points); });
    EXPECT_EQ(joined.long_count(), 0u);
}

TEST(Intersect, DisjointRangesEmpty)
{
    std::vector<int> const a{1, 2, 3};
    std::vector<int> const b{4, 5, 6};
    auto out = qb::linq::from(a).intersect(b).to_vector();
    EXPECT_EQ(out.long_count(), 0u);
}

TEST(Except, EmptySecondRangeNoRemoval)
{
    std::vector<int> const main{1, 2, 3};
    std::vector<int> const ban;
    auto out = qb::linq::from(main).except(ban).to_vector();
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(main));
}

/** Matches .NET `Enumerable.Except`: distinct left values, first-seen order (duplicates on the left collapse). */
TEST(Except, DistinctLeftFirstSeenOrder)
{
    std::vector<int> const main{1, 1, 2, 2, 3};
    std::vector<int> const ban{3};
    auto out = qb::linq::from(main).except(ban).to_vector();
    std::vector<int> const expect{1, 2};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
}

/** Matches .NET `Enumerable.Intersect`: distinct left values, first-seen order. */
TEST(Intersect, DistinctLeftFirstSeenOrder)
{
    std::vector<int> const a{3, 3, 4, 4, 5};
    std::vector<int> const b{3, 4, 5, 5};
    auto out = qb::linq::from(a).intersect(b).to_vector();
    std::vector<int> const expect{3, 4, 5};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
}

/** Heavy duplicates on both sides; compare sorted bags to a hand-rolled set reference (GPT-style regression). */
TEST(ExceptIntersect, DuplicatesOnBothSidesVsSortedSetReference)
{
    std::vector<int> const a{1, 1, 2, 2, 3, 3, 4};
    std::vector<int> const b{3, 3, 4, 4, 5, 5};

    auto got_e = qb::linq::from(a).except(b).to_vector();
    auto got_i = qb::linq::from(a).intersect(b).to_vector();

    std::vector<int> sort_e(got_e.begin(), got_e.end());
    std::vector<int> sort_i(got_i.begin(), got_i.end());
    std::sort(sort_e.begin(), sort_e.end());
    std::sort(sort_i.begin(), sort_i.end());

    std::vector<int> ua = a;
    std::vector<int> ub = b;
    std::sort(ua.begin(), ua.end());
    std::sort(ub.begin(), ub.end());
    ua.erase(std::unique(ua.begin(), ua.end()), ua.end());
    ub.erase(std::unique(ub.begin(), ub.end()), ub.end());

    std::vector<int> want_e;
    std::vector<int> want_i;
    for (int x : ua) {
        if (!std::binary_search(ub.begin(), ub.end(), x))
            want_e.push_back(x);
    }
    for (int x : ua) {
        if (std::binary_search(ub.begin(), ub.end(), x))
            want_i.push_back(x);
    }
    std::sort(want_e.begin(), want_e.end());
    std::sort(want_i.begin(), want_i.end());

    EXPECT_EQ(sort_e, want_e);
    EXPECT_EQ(sort_i, want_i);
}

TEST(LeftJoin, UnmatchedOuterYieldsEmptyOptional)
{
    std::vector<Person> const people{{1, "Ann"}, {2, "Bob"}, {3, "Cam"}};
    std::vector<Score> const scores{{1, 10}, {2, 20}};
    auto rows = qb::linq::from(people)
                    .left_join(scores,
                        [](Person const& p) { return p.id; },
                        [](Score const& s) { return s.person_id; },
                        [](Person const& p, std::optional<Score> const& os) {
                            return std::make_tuple(p.name, os ? os->points : -1);
                        });
    ASSERT_EQ(rows.count(), 3u);
    auto it = rows.begin();
    EXPECT_EQ(std::get<0>(*it), "Ann");
    EXPECT_EQ(std::get<1>(*it), 10);
    ++it;
    EXPECT_EQ(std::get<0>(*it), "Bob");
    EXPECT_EQ(std::get<1>(*it), 20);
    ++it;
    EXPECT_EQ(std::get<0>(*it), "Cam");
    EXPECT_EQ(std::get<1>(*it), -1);
}

TEST(LeftJoin, MultipleInnersMatchesInnerJoinRowCount)
{
    std::vector<Person> const people{{1, "Ann"}, {2, "Bob"}};
    std::vector<Score> const scores{{1, 5}, {1, 7}, {2, 3}};
    auto inner_n = qb::linq::from(people)
                       .join(scores,
                           [](Person const& p) { return p.id; },
                           [](Score const& s) { return s.person_id; },
                           [](Person const&, Score const& s) { return s.points; })
                       .long_count();
    auto left_n = qb::linq::from(people)
                      .left_join(scores,
                          [](Person const& p) { return p.id; },
                          [](Score const& s) { return s.person_id; },
                          [](Person const&, std::optional<Score> const& os) { return os->points; })
                      .long_count();
    EXPECT_EQ(left_n, inner_n);
    EXPECT_EQ(left_n, 3u);
}

TEST(LeftJoin, EmptyInnerRangeOneRowPerOuter)
{
    std::vector<Person> const people{{1, "Ann"}};
    std::vector<Score> const scores{};
    auto rows = qb::linq::from(people)
                    .left_join(scores,
                        [](Person const& p) { return p.id; },
                        [](Score const& s) { return s.person_id; },
                        [](Person const& p, std::optional<Score> const& os) {
                            return std::make_pair(p.id, os.has_value());
                        });
    ASSERT_EQ(rows.count(), 1u);
    EXPECT_FALSE(rows.begin()->second);
}

TEST(RightJoin, UnmatchedInnerYieldsEmptyOptionalOuter)
{
    std::vector<Person> const people{{1, "Ann"}, {2, "Bob"}};
    std::vector<Score> const scores{{1, 10}, {2, 20}, {99, 1}};
    auto rows = qb::linq::from(people)
                    .right_join(scores,
                        [](Person const& p) { return p.id; },
                        [](Score const& s) { return s.person_id; },
                        [](std::optional<Person> const& op, Score const& s) {
                            return std::make_tuple(s.person_id, s.points, op.has_value());
                        });
    ASSERT_EQ(rows.count(), 3u);
    auto it = rows.begin();
    EXPECT_TRUE(std::get<2>(*it));
    ++it;
    EXPECT_TRUE(std::get<2>(*it));
    ++it;
    EXPECT_EQ(std::get<0>(*it), 99);
    EXPECT_FALSE(std::get<2>(*it));
}

TEST(RightJoin, MultipleOutersDuplicatesInnerKey)
{
    std::vector<Person> const people{{1, "Ann"}, {1, "AnnDup"}};
    std::vector<Score> const scores{{1, 100}};
    auto rows = qb::linq::from(people)
                    .right_join(scores,
                        [](Person const& p) { return p.id; },
                        [](Score const& s) { return s.person_id; },
                        [](std::optional<Person> const& op, Score const& s) {
                            return std::make_pair(op->name, s.points);
                        });
    ASSERT_EQ(rows.count(), 2u);
}

TEST(RightJoin, EmptyOuterAllInnersUnmatched)
{
    std::vector<Person> const people{};
    std::vector<Score> const scores{{7, 1}};
    auto rows = qb::linq::from(people)
                    .right_join(scores,
                        [](Person const& p) { return p.id; },
                        [](Score const& s) { return s.person_id; },
                        [](std::optional<Person> const& op, Score const& s) {
                            return std::make_pair(op.has_value(), s.points);
                        });
    ASSERT_EQ(rows.count(), 1u);
    EXPECT_FALSE(rows.begin()->first);
    EXPECT_EQ(rows.begin()->second, 1);
}
