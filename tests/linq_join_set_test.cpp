/// @file linq_join_set_test.cpp
/// @brief Relational-style join / group_join and set union.

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
