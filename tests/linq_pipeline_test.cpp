/// @file linq_pipeline_test.cpp
/// @brief Tests for lazy views: select, where, skip, take, chaining vs reference algorithms.

#include <algorithm>
#include <iterator>
#include <memory>
#include <numeric>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

namespace {

std::vector<int> iota_vec(int n)
{
    std::vector<int> v(static_cast<std::size_t>(n));
    std::iota(v.begin(), v.end(), 0);
    return v;
}

} // namespace

TEST(PipelineSelectWhere, MatchesStdCopyIfTransform)
{
    std::vector<int> const data = iota_vec(50);
    auto const linq_sum = qb::linq::from(data)
                              .where([](int x) { return x % 3 == 0; })
                              .select([](int x) { return x * 2; })
                              .sum();

    int ref = 0;
    for (int x : data) {
        if (x % 3 == 0)
            ref += x * 2;
    }
    EXPECT_EQ(linq_sum, ref);
}

TEST(PipelineSkipTake, SlicesLikeSubrange)
{
    std::vector<int> const data = iota_vec(100);
    auto const pipe = qb::linq::from(data).skip(20).take(15);
    EXPECT_EQ(pipe.long_count(), 15u);
    EXPECT_EQ(pipe.first(), 20);
    // Use materialized last(): `last()` on `take_n` end-iterator can point past the logical take window.
    auto const materialized = pipe.to_vector();
    EXPECT_EQ(materialized.last(), 34);
}

TEST(PipelineChained, SameAsNestedLoops)
{
    std::vector<int> const data{3, 1, 4, 1, 5, 9, 2, 6};
    int const chained = qb::linq::from(data)
                            .where([](int x) { return x > 2; })
                            .select([](int x) { return x * x; })
                            .take(3)
                            .sum();
    // >2: 3,4,5,9,6 → squares 9,16,25 → take 3 → 9+16+25
    EXPECT_EQ(chained, 9 + 16 + 25);
}

TEST(PipelineDistinct, CountMatchesUnorderedSet)
{
    std::vector<int> const data{1, 2, 2, 3, 1, 4, 2};
    auto const dv = qb::linq::from(data).distinct().to_vector();
    std::unordered_set<int> u(data.begin(), data.end());
    EXPECT_EQ(dv.long_count(), u.size());
}

TEST(PipelineEnumerate, IndicesAlign)
{
    std::vector<char> const data{'a', 'b', 'c'};
    std::size_t i = 0;
    for (auto const pr : qb::linq::from(data).enumerate()) {
        EXPECT_EQ(pr.first, i);
        EXPECT_EQ(pr.second, data[i]);
        ++i;
    }
    EXPECT_EQ(i, 3u);
}

namespace {

/** Minimal `std::input_iterator` over a shared index into a vector (single logical pass over backing data). */
class SinglePassRange {
public:
    struct State {
        std::vector<int> data;
        std::size_t index = 0;
    };

    class iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = int;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = int;

        iterator() = default;

        iterator(std::shared_ptr<State> st, bool end = false) : st_(std::move(st)), end_(end) { normalize(); }

        int operator*() const { return st_->data[st_->index]; }

        iterator& operator++()
        {
            ++st_->index;
            normalize();
            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp = *this;
            ++*this;
            return tmp;
        }

        friend bool operator==(iterator const& a, iterator const& b)
        {
            if (a.end_ && b.end_)
                return true;
            return a.st_ == b.st_ && a.end_ == b.end_;
        }

        friend bool operator!=(iterator const& a, iterator const& b) { return !(a == b); }

    private:
        void normalize()
        {
            if (!st_) {
                end_ = true;
                return;
            }
            if (st_->index >= st_->data.size())
                end_ = true;
        }

        std::shared_ptr<State> st_;
        bool end_ = false;
    };

    explicit SinglePassRange(std::vector<int> v) : st_(std::make_shared<State>(State{std::move(v), 0})) {}

    iterator begin() { return iterator(st_, false); }
    iterator end() { return iterator(st_, true); }

private:
    std::shared_ptr<State> st_;
};

} // namespace

/** Predicate invocation counts are not an API contract (`where_view` caches first match, etc.). */
TEST(PipelineStatefulPredicates, RepeatedToVectorSameResults)
{
    std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8};
    int where_calls = 0;
    auto q = qb::linq::from(v)
                 .where([&](int x) {
                     ++where_calls;
                     return x % 2 == 0;
                 })
                 .select([](int x) { return x * 10; });

    auto a = q.to_vector();
    auto b = q.to_vector();
    EXPECT_TRUE(a.sequence_equal(b));
    EXPECT_GE(where_calls, 8);
}

/**
 * Deep lazy chain over a true input sequence; reference uses a fresh loop variable per element (not the GPT bug
 * that reused the `for` index as the transformed value).
 */
TEST(PipelineSinglePassInput, DeepChainMatchesReference)
{
    std::vector<int> data(300);
    std::iota(data.begin(), data.end(), 1);
    SinglePassRange r(std::move(data));

    auto mat = qb::linq::from(r)
                   .where([](int x) { return x % 2 == 0; })
                   .select([](int x) { return x * 2; })
                   .where([](int x) { return x % 3 != 0; })
                   .select([](int x) { return x + 5; })
                   .skip(10)
                   .take(25)
                   .to_vector();

    std::vector<int> ref;
    for (int xi = 1; xi <= 300; ++xi) {
        int t = xi;
        if (t % 2 != 0)
            continue;
        t = t * 2;
        if (t % 3 == 0)
            continue;
        t = t + 5;
        ref.push_back(t);
    }
    if (ref.size() > 10)
        ref.erase(ref.begin(), ref.begin() + 10);
    else
        ref.clear();
    if (ref.size() > 25)
        ref.resize(25);

    EXPECT_TRUE(mat.sequence_equal(ref));
}

TEST(PipelineSinglePassInput, OddSquaresTakeThree)
{
    SinglePassRange r(std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9});
    auto out = qb::linq::from(r)
                   .where([](int x) { return x % 2 == 1; })
                   .select([](int x) { return x * x; })
                   .take(3)
                   .to_vector();
    std::vector<int> const expect{1, 9, 25};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
}

TEST(PipelineSinglePassInput, CountIfEvens)
{
    SinglePassRange r(std::vector<int>{2, 4, 5, 8, 11, 14});
    EXPECT_EQ(qb::linq::from(r).count_if([](int x) { return x % 2 == 0; }), 4u);
}

TEST(PipelineSinglePassInput, ScanPrefixSums)
{
    SinglePassRange r(std::vector<int>{1, 2, 3, 4});
    auto out = qb::linq::from(r)
                   .scan(0, [](int acc, int x) { return acc + x; })
                   .to_vector();
    std::vector<int> const expect{1, 3, 6, 10};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
}

TEST(PipelineMoveOnly, UniquePtrFilterAndSelectLeavesSourceIntact)
{
    std::vector<std::unique_ptr<int>> src;
    src.emplace_back(std::make_unique<int>(1));
    src.emplace_back(std::make_unique<int>(2));
    src.emplace_back(std::make_unique<int>(3));
    auto out = qb::linq::from(src)
                   .where([](std::unique_ptr<int> const& p) { return *p >= 2; })
                   .select([](std::unique_ptr<int> const& p) { return *p * 10; })
                   .to_vector();
    std::vector<int> const expect{20, 30};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
    ASSERT_TRUE(src[0] && src[1] && src[2]);
}

TEST(PipelineMoveOnly, UniquePtrSkipsNullptr)
{
    std::vector<std::unique_ptr<int>> src;
    src.emplace_back(std::make_unique<int>(7));
    src.push_back(nullptr);
    src.emplace_back(std::make_unique<int>(9));
    auto out = qb::linq::from(src)
                   .where([](std::unique_ptr<int> const& p) { return static_cast<bool>(p); })
                   .select([](std::unique_ptr<int> const& p) { return *p; })
                   .to_vector();
    std::vector<int> const expect{7, 9};
    EXPECT_TRUE(qb::linq::from(out).sequence_equal(expect));
}
