/// @file linq_zip_take_where_test.cpp
/// @brief zip / zip_fold (mismatched lengths), take(negative), where_view cached begin().

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(ZipUneven, StopsAtShorterRight)
{
    std::vector<int> const left{1, 2, 3, 4};
    std::vector<int> const right{10, 20};
    std::vector<std::pair<int, int>> got;
    for (auto const& pr : qb::linq::from(left).zip(right))
        got.push_back(pr);
    ASSERT_EQ(got.size(), 2u);
    EXPECT_EQ(got[0].first, 1);
    EXPECT_EQ(got[0].second, 10);
    EXPECT_EQ(got[1].first, 2);
    EXPECT_EQ(got[1].second, 20);
}

TEST(ZipUneven, StopsAtShorterLeft)
{
    std::vector<int> const left{1, 2};
    std::vector<int> const right{100, 200, 300, 400};
    EXPECT_EQ(qb::linq::from(left).zip(right).long_count(), 2u);
}

TEST(ZipUneven, OneSideSingleton)
{
    std::vector<int> const left{7};
    std::vector<int> const right{3, 4, 5};
    auto z = qb::linq::from(left).zip(right);
    EXPECT_TRUE(z.any());
    EXPECT_EQ(z.long_count(), 1u);
    EXPECT_EQ(z.first().first, 7);
    EXPECT_EQ(z.first().second, 3);
}

TEST(ZipFold, MatchesZipSelectAggregate)
{
    std::vector<int> const left{1, 2, 3};
    std::vector<int> const right{10, 20, 30, 40};
    auto const via_zip = qb::linq::from(left)
                             .zip(right)
                             .select([](auto const& pr) {
                                 return static_cast<std::int64_t>(pr.first)
                                     * static_cast<std::int64_t>(pr.second);
                             })
                             .aggregate(std::int64_t{0}, [](std::int64_t a, std::int64_t x) { return a + x; });
    auto const via_fold = qb::linq::from(left).zip_fold(
        right,
        std::int64_t{0},
        [](std::int64_t acc, int x, int y) {
            return acc + static_cast<std::int64_t>(x) * static_cast<std::int64_t>(y);
        });
    EXPECT_EQ(via_fold, via_zip);
    EXPECT_EQ(via_fold, 1 * 10 + 2 * 20 + 3 * 30);
}

TEST(ZipFold, BothEmptyReturnsSeedUnchanged)
{
    std::vector<int> const a;
    std::vector<int> const b;
    int const out = qb::linq::from(a).zip_fold(
        b,
        99,
        [](int acc, int, int) {
            return acc;
        });
    EXPECT_EQ(out, 99);
}

TEST(TakeNegative, SameAsPositiveMagnitude)
{
    std::vector<int> const data{1, 2, 3, 4, 5};
    auto a = qb::linq::from(data).take(-3).to_vector();
    auto b = qb::linq::from(data).take(3).to_vector();
    EXPECT_TRUE(qb::linq::from(a).sequence_equal(b));
}

TEST(WhereCachedBegin, PredicateNotReRunOnSecondBegin)
{
    std::vector<int> const data{1, 3, 5, 8, 9};
    int calls = 0;
    auto q = qb::linq::from(data).where([&calls](int x) {
        ++calls;
        return x % 2 == 0;
    });
    (void)q.begin();
    int const after_first = calls;
    (void)q.begin();
    EXPECT_EQ(calls, after_first);
    EXPECT_GE(after_first, 1);
}

TEST(WhereEvenSum, MatchesHandFilter)
{
    std::vector<int> const data{2, 4, 5, 6};
    int const s = qb::linq::from(data).where([](int x) { return x % 2 == 0; }).sum();
    EXPECT_EQ(s, 2 + 4 + 6);
}
