/// @file linq_scan_and_iterator_layout_test.cpp
/// @brief `scan_view` iterator sharing / copy semantics, stateful fold functors, and empty-base optimization
///        layout checks (`detail::compressed_fn`). `sizeof` checks use a minimal RA iterator (one pointer) so
///        they are stable across libstdc++/libc++/MSVC STL debug iterator layouts.

#include <array>
#include <cstddef>
#include <numeric>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"
#include "qb/linq/iterators.h"

namespace {

// --- Empty class functors (portable EBO targets; not lambdas) ---

struct EmptySelectFn {
    int operator()(int x) const { return x + 1; }
};

struct EmptyWherePred {
    bool operator()(int x) const { return x % 2 == 0; }
};

struct EmptyTakeWhilePred {
    bool operator()(int) const { return true; }
};

static_assert(std::is_empty_v<EmptySelectFn>);
static_assert(std::is_empty_v<EmptyWherePred>);
static_assert(std::is_empty_v<EmptyTakeWhilePred>);

using VecIt = std::vector<int>::iterator;

/**
 * @brief Single-pointer random-access iterator — `sizeof` equals `sizeof(int*)` on all tested ABIs.
 * @note Used only for layout tests (`select_iterator` cannot inherit a raw pointer type).
 */
struct MinimalRandomAccessIt {
    int* p{};

    using iterator_category = std::random_access_iterator_tag;
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using pointer = int*;
    using reference = int&;

    reference operator*() const { return *p; }
    MinimalRandomAccessIt& operator++()
    {
        ++p;
        return *this;
    }
    MinimalRandomAccessIt operator++(int)
    {
        auto t = *this;
        ++*this;
        return t;
    }
    MinimalRandomAccessIt& operator--()
    {
        --p;
        return *this;
    }
    MinimalRandomAccessIt operator--(int)
    {
        auto t = *this;
        --*this;
        return t;
    }
    bool operator==(MinimalRandomAccessIt o) const { return p == o.p; }
    bool operator!=(MinimalRandomAccessIt o) const { return !(*this == o); }
    difference_type operator-(MinimalRandomAccessIt o) const { return p - o.p; }
    MinimalRandomAccessIt& operator+=(difference_type d)
    {
        p += d;
        return *this;
    }
    MinimalRandomAccessIt& operator-=(difference_type d)
    {
        p -= d;
        return *this;
    }
    MinimalRandomAccessIt operator+(difference_type d) const { return MinimalRandomAccessIt{p + d}; }
    friend MinimalRandomAccessIt operator+(difference_type d, MinimalRandomAccessIt i) { return i + d; }
    reference operator[](difference_type d) const { return p[d]; }
};

static_assert(sizeof(MinimalRandomAccessIt) == sizeof(int*));

/** Non-empty projection; large enough that `sizeof(select_iterator<..., FatSelectProj>)` exceeds the empty-EBO case on every platform. */
struct FatSelectProj {
    std::array<std::byte, 40> blob{};
    int operator()(int x) const noexcept
    {
        return x + static_cast<int>(blob[0]);
    }
};

static_assert(!std::is_empty_v<FatSelectProj>);
static_assert(sizeof(FatSelectProj) >= 40);

struct StatefulWherePred {
    long long pad{0};
    bool operator()(int x) const { return x % 2 == 0; }
};

} // namespace

// Authoritative EBO checks: minimal base iterator (not STL `vector::iterator`, which may be debug-fat).
TEST(IteratorLayoutEbo, SelectIteratorEmptyFunctorSameSizeAsMinimalBase)
{
    using Sel = qb::linq::select_iterator<MinimalRandomAccessIt, EmptySelectFn>;
    EXPECT_EQ(sizeof(Sel), sizeof(MinimalRandomAccessIt)) << "empty functor should EBO (MSVC: empty_bases on adaptors)";
}

TEST(IteratorLayoutEbo, TakeWhileIteratorEmptyFunctorSameSizeAsMinimalBase)
{
    using Tw = qb::linq::take_while_iterator<MinimalRandomAccessIt, EmptyTakeWhilePred>;
    EXPECT_EQ(sizeof(Tw), sizeof(MinimalRandomAccessIt));
}

TEST(IteratorLayoutEbo, WhereIteratorEmptyVsStatefulOnMinimalBase)
{
    using Wi = qb::linq::where_iterator<MinimalRandomAccessIt, EmptyWherePred>;
    using WiStateful = qb::linq::where_iterator<MinimalRandomAccessIt, StatefulWherePred>;
    EXPECT_LE(sizeof(Wi), sizeof(WiStateful));
}

TEST(IteratorLayoutEbo, SelectFatFunctorLargerThanEmptyOnMinimalBase)
{
    using SelEmpty = qb::linq::select_iterator<MinimalRandomAccessIt, EmptySelectFn>;
    using SelFat = qb::linq::select_iterator<MinimalRandomAccessIt, FatSelectProj>;
    EXPECT_GT(sizeof(SelFat), sizeof(SelEmpty));
}

// `std::vector` iterators: must match when the STL uses a pointer-sized handle (MSVC release, typical libstdc++).
TEST(IteratorLayoutEbo, SelectIteratorMatchesVectorIteratorWhenStlIteratorIsPointerSized)
{
    using Sel = qb::linq::select_iterator<VecIt, EmptySelectFn>;
    if (sizeof(VecIt) != sizeof(int*))
        GTEST_SKIP() << "STL iterator is not pointer-sized (e.g. debug / proxy); minimal-iterator tests cover EBO.";
    EXPECT_EQ(sizeof(Sel), sizeof(VecIt));
}

TEST(IteratorLayoutEbo, TakeWhileIteratorMatchesVectorIteratorWhenStlIteratorIsPointerSized)
{
    using Tw = qb::linq::take_while_iterator<VecIt, EmptyTakeWhilePred>;
    if (sizeof(VecIt) != sizeof(int*))
        GTEST_SKIP() << "STL iterator is not pointer-sized; minimal-iterator tests cover EBO.";
    EXPECT_EQ(sizeof(Tw), sizeof(VecIt));
}

TEST(IteratorLayoutEbo, WhereIteratorNoExtraStorageForEmptyPredBeyondThreeIteratorsWorth)
{
    using Wi = qb::linq::where_iterator<VecIt, EmptyWherePred>;
    using WiStateful = qb::linq::where_iterator<VecIt, StatefulWherePred>;
    EXPECT_LE(sizeof(Wi), sizeof(WiStateful));
}

TEST(IteratorLayoutEbo, SelectFatFunctorLargerThanEmptyOnVectorIterator)
{
    using SelEmpty = qb::linq::select_iterator<VecIt, EmptySelectFn>;
    using SelFat = qb::linq::select_iterator<VecIt, FatSelectProj>;
    EXPECT_GT(sizeof(SelFat), sizeof(SelEmpty));
}

// --- scan: iterator copies share one fold function object in the view ---

TEST(ScanContract, CopiedIteratorsSeeSameRunningFold)
{
    std::vector<int> const data{2, 3, 5};
    auto rng = qb::linq::from(data).scan(10, [](int a, int b) { return a + b; });
    auto a = rng.begin();
    auto b = a;
    ASSERT_NE(a, rng.end());
    EXPECT_EQ(*a, *b);
    EXPECT_EQ(*a, 12);
    ++a;
    EXPECT_EQ(*a, 15);
    // `b` still at first position; must still read first partial sum
    EXPECT_EQ(*b, 12);
    ++b;
    EXPECT_EQ(*b, 15);
}

TEST(ScanContract, PostIncrementObservesSameSequenceAsPreIncrementWalk)
{
    std::vector<int> const data{1, 1, 1};
    std::vector<int> pre;
    {
        auto rng = qb::linq::from(data).scan(0, [](int x, int y) { return x + y; });
        for (auto it = rng.begin(), e = rng.end(); it != e; ++it)
            pre.push_back(*it);
    }
    std::vector<int> post;
    {
        auto rng = qb::linq::from(data).scan(0, [](int x, int y) { return x + y; });
        for (auto it = rng.begin(), e = rng.end(); it != e;)
            post.push_back(*it++);
    }
    std::vector<int> const expect{1, 2, 3};
    EXPECT_EQ(pre, expect);
    EXPECT_EQ(post, expect);
}

TEST(ScanContract, CopiedEnumerableProducesSameOutput)
{
    std::vector<int> const data{4, 5};
    auto a = qb::linq::from(data).scan(100, [](int acc, int v) { return acc - v; });
    auto b = a;
    std::vector<int> va;
    std::vector<int> vb;
    for (int x : a.to_vector())
        va.push_back(x);
    for (int x : b.to_vector())
        vb.push_back(x);
    std::vector<int> const expect{96, 91};
    EXPECT_EQ(va, expect);
    EXPECT_EQ(vb, expect);
}

TEST(ScanContract, StatefulFunctorObjectTracksInvocations)
{
    struct Fold {
        int* calls{};
        int operator()(int acc, int v)
        {
            ++(*calls);
            return acc + v * 2;
        }
    };

    std::vector<int> const data{1, 2, 3};
    int n = 0;
    Fold fn{&n};
    std::vector<int> out;
    {
        auto rng = qb::linq::from(data).scan(0, fn);
        for (int x : rng)
            out.push_back(x);
    }
    std::vector<int> const expect{2, 6, 12};
    EXPECT_EQ(out, expect);
    EXPECT_EQ(n, 3);
}

TEST(ScanContract, MatchesStdPartialSumStyleForInts)
{
    std::vector<int> const data{3, 1, 4, 1, 5};
    std::vector<int> linq_out;
    for (int x : qb::linq::from(data).scan(0, [](int a, int b) { return a + b; }))
        linq_out.push_back(x);

    std::vector<int> partial;
    std::partial_sum(data.begin(), data.end(), std::back_inserter(partial));
    EXPECT_EQ(linq_out, partial);
}

TEST(ScanContract, SelectWithEmptyClassFunctorMatchesLambda)
{
    std::vector<int> const data{1, 2, 3};
    std::vector<int> with_struct;
    std::vector<int> with_lambda;
    for (int x : qb::linq::from(data).select(EmptySelectFn{}).to_vector())
        with_struct.push_back(x);
    for (int x : qb::linq::from(data).select([](int x) { return x + 1; }).to_vector())
        with_lambda.push_back(x);
    std::vector<int> const expect{2, 3, 4};
    EXPECT_EQ(with_struct, expect);
    EXPECT_EQ(with_lambda, expect);
}

TEST(ScanContract, WhereWithEmptyClassFunctor)
{
    std::vector<int> const data{1, 2, 3, 4};
    std::vector<int> out;
    for (int x : qb::linq::from(data).where(EmptyWherePred{}).to_vector())
        out.push_back(x);
    std::vector<int> const expect{2, 4};
    EXPECT_EQ(out, expect);
}
