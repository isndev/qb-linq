/**
 * @file linq_new_features_test.cpp
 * @brief Tests for new features: flat_map, sliding_window, cast<T>(), aggregate_by, reduce_by.
 */

#include <qb/linq.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

// ===========================================================================
// flat_map / select_many_flatten — lazy one-to-many projection + flatten
// ===========================================================================

TEST(FlatMap, BasicFlatten)
{
    std::vector<std::vector<int>> data{{1, 2}, {3}, {4, 5, 6}};
    auto result = qb::linq::from(data)
                      .flat_map([](auto const& v) -> auto const& { return v; })
                      .to_vector();
    std::vector<int> expected{1, 2, 3, 4, 5, 6};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(FlatMap, EmptyOuterRange)
{
    std::vector<std::vector<int>> data;
    auto result = qb::linq::from(data)
                      .flat_map([](auto const& v) -> auto const& { return v; })
                      .to_vector();
    EXPECT_EQ(result.count(), 0u);
}

TEST(FlatMap, EmptyInnerRanges)
{
    std::vector<std::vector<int>> data{{}, {}, {}};
    auto result = qb::linq::from(data)
                      .flat_map([](auto const& v) -> auto const& { return v; })
                      .to_vector();
    EXPECT_EQ(result.count(), 0u);
}

TEST(FlatMap, MixedEmptyAndNonEmpty)
{
    std::vector<std::vector<int>> data{{}, {1, 2}, {}, {3}, {}};
    auto result = qb::linq::from(data)
                      .flat_map([](auto const& v) -> auto const& { return v; })
                      .to_vector();
    std::vector<int> expected{1, 2, 3};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(FlatMap, ProjectionGeneratesRange)
{
    // Each integer N generates {N, N*10}
    std::vector<int> data{1, 2, 3};
    auto result = qb::linq::from(data)
                      .flat_map([](int x) { return std::vector<int>{x, x * 10}; })
                      .to_vector();
    std::vector<int> expected{1, 10, 2, 20, 3, 30};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(FlatMap, StringSplit)
{
    // Flatten words into characters
    std::vector<std::string> words{"ab", "cd", "e"};
    auto result = qb::linq::from(words)
                      .flat_map([](std::string const& w) -> auto const& { return w; })
                      .to_vector();
    std::vector<char> expected{'a', 'b', 'c', 'd', 'e'};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(FlatMap, ChainedWithWhere)
{
    std::vector<std::vector<int>> data{{1, 2, 3}, {4, 5, 6}};
    auto result = qb::linq::from(data)
                      .flat_map([](auto const& v) -> auto const& { return v; })
                      .where([](int x) { return x % 2 == 0; })
                      .to_vector();
    std::vector<int> expected{2, 4, 6};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(FlatMap, SingleElementInner)
{
    std::vector<std::vector<int>> data{{1}, {2}, {3}};
    auto result = qb::linq::from(data)
                      .flat_map([](auto const& v) -> auto const& { return v; })
                      .to_vector();
    std::vector<int> expected{1, 2, 3};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(FlatMap, LazyEvaluation)
{
    int outer_calls = 0;
    std::vector<std::vector<int>> data{{1, 2}, {3, 4}, {5, 6}};
    auto q = qb::linq::from(data).flat_map([&](auto const& v) -> auto const& {
        ++outer_calls;
        return v;
    });

    EXPECT_EQ(outer_calls, 0); // nothing evaluated yet

    auto it = q.begin();
    EXPECT_GE(outer_calls, 1); // first outer element evaluated
    EXPECT_EQ(*it, 1);
}

TEST(FlatMap, SelectManyFlattenAlias)
{
    std::vector<std::vector<int>> data{{1, 2}, {3}};
    auto result = qb::linq::from(data)
                      .select_many_flatten([](auto const& v) -> auto const& { return v; })
                      .to_vector();
    std::vector<int> expected{1, 2, 3};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(FlatMap, CountOnFlatMap)
{
    std::vector<std::vector<int>> data{{1, 2}, {3, 4, 5}};
    auto count = qb::linq::from(data)
                     .flat_map([](auto const& v) -> auto const& { return v; })
                     .count();
    EXPECT_EQ(count, 5u);
}

TEST(FlatMap, SumOnFlatMap)
{
    std::vector<std::vector<int>> data{{1, 2}, {3}};
    auto sum = qb::linq::from(data)
                   .flat_map([](auto const& v) -> auto const& { return v; })
                   .sum();
    EXPECT_EQ(sum, 6);
}

// ===========================================================================
// sliding_window — overlapping windows of size N
// ===========================================================================

TEST(SlidingWindow, BasicWindow)
{
    std::vector<int> data{1, 2, 3, 4, 5};
    auto windows = qb::linq::from(data).sliding_window(3);
    auto it = windows.begin();

    // Window 1: {1, 2, 3}
    EXPECT_EQ((*it).size(), 3u);
    EXPECT_EQ((*it)[0], 1);
    EXPECT_EQ((*it)[1], 2);
    EXPECT_EQ((*it)[2], 3);

    ++it;
    // Window 2: {2, 3, 4}
    EXPECT_EQ((*it).size(), 3u);
    EXPECT_EQ((*it)[0], 2);
    EXPECT_EQ((*it)[1], 3);
    EXPECT_EQ((*it)[2], 4);

    ++it;
    // Window 3: {3, 4, 5}
    EXPECT_EQ((*it).size(), 3u);
    EXPECT_EQ((*it)[0], 3);
    EXPECT_EQ((*it)[1], 4);
    EXPECT_EQ((*it)[2], 5);

    ++it;
    // Window 4: {4, 5} (partial)
    EXPECT_EQ((*it).size(), 2u);
    EXPECT_EQ((*it)[0], 4);
    EXPECT_EQ((*it)[1], 5);

    ++it;
    // Window 5: {5} (partial)
    EXPECT_EQ((*it).size(), 1u);
    EXPECT_EQ((*it)[0], 5);

    ++it;
    EXPECT_EQ(it, windows.end());
}

TEST(SlidingWindow, EmptySequence)
{
    std::vector<int> empty;
    EXPECT_EQ(qb::linq::from(empty).sliding_window(3).count(), 0u);
}

TEST(SlidingWindow, SingleElement)
{
    std::vector<int> data{42};
    auto windows = qb::linq::from(data).sliding_window(3);
    EXPECT_EQ(windows.count(), 1u);
    auto it = windows.begin();
    EXPECT_EQ((*it).size(), 1u);
    EXPECT_EQ((*it)[0], 42);
}

TEST(SlidingWindow, WindowSizeOne)
{
    std::vector<int> data{1, 2, 3};
    auto windows = qb::linq::from(data).sliding_window(1);
    EXPECT_EQ(windows.count(), 3u);
    auto it = windows.begin();
    EXPECT_EQ((*it).size(), 1u);
    EXPECT_EQ((*it)[0], 1);
}

TEST(SlidingWindow, WindowSizeEqualsSequence)
{
    std::vector<int> data{1, 2, 3};
    auto windows = qb::linq::from(data).sliding_window(3);
    auto it = windows.begin();
    EXPECT_EQ((*it).size(), 3u);
    EXPECT_EQ((*it)[0], 1);
    EXPECT_EQ((*it)[1], 2);
    EXPECT_EQ((*it)[2], 3);
}

TEST(SlidingWindow, WindowSizeLargerThanSequence)
{
    std::vector<int> data{1, 2};
    auto windows = qb::linq::from(data).sliding_window(5);
    EXPECT_EQ(windows.count(), 2u);
    auto it = windows.begin();
    EXPECT_EQ((*it).size(), 2u);
}

TEST(SlidingWindow, WindowSizeZeroClamped)
{
    std::vector<int> data{1, 2, 3};
    auto windows = qb::linq::from(data).sliding_window(0);
    // Clamped to 1
    EXPECT_EQ(windows.count(), 3u);
}

TEST(SlidingWindow, MovingAverage)
{
    std::vector<double> data{1.0, 2.0, 3.0, 4.0, 5.0};
    auto avgs = qb::linq::from(data)
                    .sliding_window(3)
                    .select([](auto const& w) {
                        double sum = 0;
                        for (auto v : w) sum += v;
                        return sum / static_cast<double>(w.size());
                    })
                    .to_vector();
    // Windows: {1,2,3}→2.0, {2,3,4}→3.0, {3,4,5}→4.0, {4,5}→4.5, {5}→5.0
    EXPECT_NEAR(avgs.element_at(0), 2.0, 0.001);
    EXPECT_NEAR(avgs.element_at(1), 3.0, 0.001);
    EXPECT_NEAR(avgs.element_at(2), 4.0, 0.001);
}

TEST(SlidingWindow, CountWithChaining)
{
    std::vector<int> data{1, 2, 3, 4, 5};
    auto count = qb::linq::from(data)
                     .sliding_window(2)
                     .where([](auto const& w) { return w.size() == 2; })
                     .count();
    EXPECT_EQ(count, 4u); // {1,2}, {2,3}, {3,4}, {4,5}
}

// ===========================================================================
// cast<T>() — static cast of each element
// ===========================================================================

TEST(Cast, IntToDouble)
{
    std::vector<int> data{1, 2, 3};
    auto result = qb::linq::from(data).cast<double>().to_vector();
    EXPECT_NEAR(result.element_at(0), 1.0, 0.001);
    EXPECT_NEAR(result.element_at(1), 2.0, 0.001);
    EXPECT_NEAR(result.element_at(2), 3.0, 0.001);
}

TEST(Cast, DoubleToInt)
{
    std::vector<double> data{1.5, 2.7, 3.1};
    auto result = qb::linq::from(data).cast<int>().to_vector();
    EXPECT_EQ(result.element_at(0), 1);
    EXPECT_EQ(result.element_at(1), 2);
    EXPECT_EQ(result.element_at(2), 3);
}

TEST(Cast, EmptySequence)
{
    std::vector<int> empty;
    EXPECT_EQ(qb::linq::from(empty).cast<double>().count(), 0u);
}

TEST(Cast, ChainedWithWhere)
{
    std::vector<int> data{1, 2, 3, 4, 5};
    auto result = qb::linq::from(data)
                      .cast<double>()
                      .where([](double x) { return x > 2.5; })
                      .to_vector();
    std::vector<double> expected{3.0, 4.0, 5.0};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(Cast, IsLazy)
{
    int call_count = 0;
    std::vector<int> data{1, 2, 3, 4, 5};
    auto q = qb::linq::from(data).select([&](int x) {
        ++call_count;
        return x;
    }).cast<double>();

    EXPECT_EQ(call_count, 0);
    (void)q.first();
    EXPECT_EQ(call_count, 1);
}

// ===========================================================================
// aggregate_by — group by key and reduce each group in one pass
// ===========================================================================

TEST(AggregateBy, SumByKey)
{
    std::vector<std::pair<std::string, int>> data{
        {"a", 1}, {"b", 2}, {"a", 3}, {"b", 4}, {"c", 5}};
    auto result = qb::linq::from(data)
                      .aggregate_by(
                          [](auto const& p) { return p.first; },
                          0,
                          [](int acc, auto const& p) { return acc + p.second; });
    // First-seen order: a → 4, b → 6, c → 5
    EXPECT_EQ(result.count(), 3u);
    EXPECT_EQ(result.element_at(0).first, "a");
    EXPECT_EQ(result.element_at(0).second, 4);
    EXPECT_EQ(result.element_at(1).first, "b");
    EXPECT_EQ(result.element_at(1).second, 6);
    EXPECT_EQ(result.element_at(2).first, "c");
    EXPECT_EQ(result.element_at(2).second, 5);
}

TEST(AggregateBy, EmptySequence)
{
    std::vector<std::pair<std::string, int>> empty;
    auto result = qb::linq::from(empty)
                      .aggregate_by(
                          [](auto const& p) { return p.first; },
                          0,
                          [](int acc, auto const& p) { return acc + p.second; });
    EXPECT_EQ(result.count(), 0u);
}

TEST(AggregateBy, SingleElement)
{
    std::vector<std::pair<std::string, int>> data{{"x", 42}};
    auto result = qb::linq::from(data)
                      .aggregate_by(
                          [](auto const& p) { return p.first; },
                          0,
                          [](int acc, auto const& p) { return acc + p.second; });
    EXPECT_EQ(result.count(), 1u);
    EXPECT_EQ(result.element_at(0).first, "x");
    EXPECT_EQ(result.element_at(0).second, 42);
}

TEST(AggregateBy, AllSameKey)
{
    std::vector<int> data{1, 2, 3, 4, 5};
    auto result = qb::linq::from(data)
                      .aggregate_by(
                          [](int) { return std::string("all"); },
                          0,
                          [](int acc, int x) { return acc + x; });
    EXPECT_EQ(result.count(), 1u);
    EXPECT_EQ(result.element_at(0).second, 15);
}

TEST(AggregateBy, AllDifferentKeys)
{
    std::vector<int> data{1, 2, 3};
    auto result = qb::linq::from(data)
                      .aggregate_by(
                          [](int x) { return x; },
                          0,
                          [](int acc, int x) { return acc + x; });
    EXPECT_EQ(result.count(), 3u);
    EXPECT_EQ(result.element_at(0).second, 1);
    EXPECT_EQ(result.element_at(1).second, 2);
    EXPECT_EQ(result.element_at(2).second, 3);
}

TEST(AggregateBy, StringConcatenation)
{
    struct Record { int category; std::string name; };
    std::vector<Record> data{
        {1, "a"}, {2, "b"}, {1, "c"}, {2, "d"}, {1, "e"}};
    auto result = qb::linq::from(data)
                      .aggregate_by(
                          [](Record const& r) { return r.category; },
                          std::string{},
                          [](std::string acc, Record const& r) { return acc + r.name; });
    EXPECT_EQ(result.count(), 2u);
    // Cat 1: "ace", Cat 2: "bd"
    EXPECT_EQ(result.element_at(0).first, 1);
    EXPECT_EQ(result.element_at(0).second, "ace");
    EXPECT_EQ(result.element_at(1).first, 2);
    EXPECT_EQ(result.element_at(1).second, "bd");
}

TEST(AggregateBy, CountByEquivalent)
{
    std::vector<int> data{1, 2, 1, 3, 2, 1};
    auto result = qb::linq::from(data)
                      .aggregate_by(
                          [](int x) { return x; },
                          0,
                          [](int acc, int) { return acc + 1; });
    // First-seen order: 1→3, 2→2, 3→1
    EXPECT_EQ(result.count(), 3u);
    EXPECT_EQ(result.element_at(0).second, 3);
    EXPECT_EQ(result.element_at(1).second, 2);
    EXPECT_EQ(result.element_at(2).second, 1);
}

TEST(AggregateBy, PreservesFirstSeenOrder)
{
    std::vector<int> data{3, 1, 2, 3, 1, 2};
    auto result = qb::linq::from(data)
                      .aggregate_by(
                          [](int x) { return x; },
                          0,
                          [](int acc, int x) { return acc + x; });
    // First-seen: 3, 1, 2
    EXPECT_EQ(result.element_at(0).first, 3);
    EXPECT_EQ(result.element_at(1).first, 1);
    EXPECT_EQ(result.element_at(2).first, 2);
}

// ===========================================================================
// reduce_by — group by key and fold without seed
// ===========================================================================

TEST(ReduceBy, SumByKey)
{
    std::vector<std::pair<int, int>> data{{1, 10}, {2, 20}, {1, 30}, {2, 40}};
    auto result = qb::linq::from(data)
                      .reduce_by(
                          [](auto const& p) { return p.first; },
                          [](auto acc, auto const& p) {
                              return std::make_pair(acc.first, acc.second + p.second);
                          });
    EXPECT_EQ(result.count(), 2u);
    // Key 1: first elem {1,10}, reduced with {1,30} → {1,40}
    EXPECT_EQ(result.element_at(0).first, 1);
    EXPECT_EQ(result.element_at(0).second.second, 40);
    // Key 2: first elem {2,20}, reduced with {2,40} → {2,60}
    EXPECT_EQ(result.element_at(1).first, 2);
    EXPECT_EQ(result.element_at(1).second.second, 60);
}

TEST(ReduceBy, SimpleIntSum)
{
    std::vector<int> data{1, 2, 3, 1, 2, 1};
    auto result = qb::linq::from(data)
                      .reduce_by(
                          [](int x) { return x % 2; }, // even/odd
                          [](int acc, int x) { return acc + x; });
    EXPECT_EQ(result.count(), 2u);
    // odd (key 1): 1+3+1+1 = 6
    // even (key 0): 2+2 = 4
    EXPECT_EQ(result.element_at(0).first, 1); // first-seen: 1 (odd)
    EXPECT_EQ(result.element_at(0).second, 6);
    EXPECT_EQ(result.element_at(1).first, 0); // then: 0 (even)
    EXPECT_EQ(result.element_at(1).second, 4);
}

TEST(ReduceBy, EmptySequence)
{
    std::vector<int> empty;
    auto result = qb::linq::from(empty)
                      .reduce_by(
                          [](int x) { return x; },
                          [](int a, int b) { return a + b; });
    EXPECT_EQ(result.count(), 0u);
}

TEST(ReduceBy, SingleElement)
{
    std::vector<int> data{42};
    auto result = qb::linq::from(data)
                      .reduce_by(
                          [](int x) { return x; },
                          [](int a, int b) { return a + b; });
    EXPECT_EQ(result.count(), 1u);
    EXPECT_EQ(result.element_at(0).first, 42);
    EXPECT_EQ(result.element_at(0).second, 42);
}

TEST(ReduceBy, AllSameKey)
{
    std::vector<int> data{10, 20, 30};
    auto result = qb::linq::from(data)
                      .reduce_by(
                          [](int) { return 0; }, // all same key
                          [](int acc, int x) { return acc + x; });
    EXPECT_EQ(result.count(), 1u);
    EXPECT_EQ(result.element_at(0).second, 60);
}

TEST(ReduceBy, MaxByKey)
{
    std::vector<int> data{3, 1, 4, 1, 5, 9, 2, 6};
    auto result = qb::linq::from(data)
                      .reduce_by(
                          [](int x) { return x % 2; }, // even/odd
                          [](int acc, int x) { return (std::max)(acc, x); });
    // Odd max: max(3,1,1,5,9) = 9
    // Even max: max(4,2,6) = 6
    auto odd_it = qb::linq::from(result).first_if([](auto const& p) { return p.first == 1; });
    auto even_it = qb::linq::from(result).first_if([](auto const& p) { return p.first == 0; });
    EXPECT_EQ(odd_it.second, 9);
    EXPECT_EQ(even_it.second, 6);
}
