/**
 * @file linq_audit_test.cpp
 * @brief Comprehensive audit tests: forward-only last(), non-default-constructible types,
 *        edge cases, repeated enumeration, append/prepend, enumerate, chunk, stride.
 */

#include <qb/linq.h>

#include <gtest/gtest.h>

#include <forward_list>
#include <list>
#include <numeric>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// last() now works on forward-only iterators
// ---------------------------------------------------------------------------

TEST(AuditLast, ForwardOnlyLastThrowsOnEmpty)
{
    std::vector<int> a;
    std::vector<int> b;
    EXPECT_THROW((void)qb::linq::from(a).concat(b).last(), std::out_of_range);
}

TEST(AuditLast, ForwardOnlyLastReturnsByValue)
{
    std::vector<int> a{1, 2, 3};
    std::vector<int> b{4, 5};
    auto result = qb::linq::from(a).concat(b).last();
    EXPECT_EQ(result, 5);
}

TEST(AuditLast, ForwardOnlyLastSingleElement)
{
    std::vector<int> a{42};
    std::vector<int> b;
    auto result = qb::linq::from(a).concat(b).last();
    EXPECT_EQ(result, 42);
}

TEST(AuditLast, BidirectionalLastReturnsReference)
{
    std::vector<int> data{10, 20, 30};
    auto& ref = qb::linq::from(data).last();
    EXPECT_EQ(ref, 30);
    ref = 99;
    EXPECT_EQ(data.back(), 99);
}

// ---------------------------------------------------------------------------
// last_if / single_if no longer require default-constructible value_type
// ---------------------------------------------------------------------------

namespace {
struct NonDefaultConstructible {
    int value;
    explicit NonDefaultConstructible(int v) : value(v) {}
    NonDefaultConstructible(NonDefaultConstructible const&) = default;
    NonDefaultConstructible& operator=(NonDefaultConstructible const&) = default;
    bool operator==(NonDefaultConstructible const& o) const { return value == o.value; }
};
} // namespace

TEST(AuditNonDefaultConstructible, LastIfWorks)
{
    std::vector<NonDefaultConstructible> data{
        NonDefaultConstructible(1), NonDefaultConstructible(2), NonDefaultConstructible(3)};
    auto result = qb::linq::from(data).last_if([](auto const& x) { return x.value > 1; });
    EXPECT_EQ(result.value, 3);
}

TEST(AuditNonDefaultConstructible, LastIfThrowsOnNoMatch)
{
    std::vector<NonDefaultConstructible> data{NonDefaultConstructible(1)};
    EXPECT_THROW(
        (void)qb::linq::from(data).last_if([](auto const& x) { return x.value > 10; }), std::out_of_range);
}

TEST(AuditNonDefaultConstructible, SingleIfWorks)
{
    std::vector<NonDefaultConstructible> data{
        NonDefaultConstructible(1), NonDefaultConstructible(2), NonDefaultConstructible(3)};
    auto result = qb::linq::from(data).single_if([](auto const& x) { return x.value == 2; });
    EXPECT_EQ(result.value, 2);
}

TEST(AuditNonDefaultConstructible, SingleIfThrowsOnMultipleMatches)
{
    std::vector<NonDefaultConstructible> data{
        NonDefaultConstructible(1), NonDefaultConstructible(2), NonDefaultConstructible(2)};
    EXPECT_THROW(
        (void)qb::linq::from(data).single_if([](auto const& x) { return x.value == 2; }), std::out_of_range);
}

TEST(AuditNonDefaultConstructible, SingleIfThrowsOnNoMatch)
{
    std::vector<NonDefaultConstructible> data{NonDefaultConstructible(1)};
    EXPECT_THROW(
        (void)qb::linq::from(data).single_if([](auto const& x) { return x.value > 10; }), std::out_of_range);
}

// ---------------------------------------------------------------------------
// Append / Prepend comprehensive tests
// ---------------------------------------------------------------------------

TEST(AuditAppendPrepend, AppendToEmpty)
{
    std::vector<int> empty;
    auto result = qb::linq::from(empty).append(42).to_vector();
    ASSERT_EQ(result.count(), 1u);
    EXPECT_EQ(result.first(), 42);
}

TEST(AuditAppendPrepend, PrependToEmpty)
{
    std::vector<int> empty;
    auto result = qb::linq::from(empty).prepend(99).to_vector();
    ASSERT_EQ(result.count(), 1u);
    EXPECT_EQ(result.first(), 99);
}

TEST(AuditAppendPrepend, AppendMultiple)
{
    std::vector<int> data{1, 2, 3};
    auto result = qb::linq::from(data).append(4).append(5).to_vector();
    std::vector<int> expected{1, 2, 3, 4, 5};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(AuditAppendPrepend, PrependMultiple)
{
    std::vector<int> data{3, 4, 5};
    auto result = qb::linq::from(data).prepend(2).prepend(1).to_vector();
    std::vector<int> expected{1, 2, 3, 4, 5};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(AuditAppendPrepend, AppendAndPrependCombined)
{
    std::vector<int> data{2, 3};
    auto result = qb::linq::from(data).prepend(1).append(4).to_vector();
    std::vector<int> expected{1, 2, 3, 4};
    EXPECT_TRUE(result.sequence_equal(expected));
}

// ---------------------------------------------------------------------------
// Enumerate comprehensive tests
// ---------------------------------------------------------------------------

TEST(AuditEnumerate, EmptySequence)
{
    std::vector<int> empty;
    EXPECT_EQ(qb::linq::from(empty).enumerate().count(), 0u);
}

TEST(AuditEnumerate, IndicesAreCorrect)
{
    std::vector<std::string> data{"a", "b", "c"};
    std::size_t idx = 0;
    qb::linq::from(data).enumerate().each([&](auto const& p) {
        EXPECT_EQ(p.first, idx);
        EXPECT_EQ(p.second, data[idx]);
        ++idx;
    });
    EXPECT_EQ(idx, 3u);
}

TEST(AuditEnumerate, SingleElement)
{
    std::vector<int> data{42};
    auto e = qb::linq::from(data).enumerate();
    auto it = e.begin();
    EXPECT_EQ((*it).first, 0u);
    EXPECT_EQ((*it).second, 42);
}

TEST(AuditEnumerate, WithWhereFilter)
{
    std::vector<int> data{10, 20, 30, 40, 50};
    auto result = qb::linq::from(data)
                      .enumerate()
                      .where([](auto const& p) { return p.first % 2 == 0; })
                      .select([](auto const& p) { return p.second; })
                      .to_vector();
    std::vector<int> expected{10, 30, 50};
    EXPECT_TRUE(result.sequence_equal(expected));
}

// ---------------------------------------------------------------------------
// Chunk comprehensive tests
// ---------------------------------------------------------------------------

TEST(AuditChunk, EmptySequence)
{
    std::vector<int> empty;
    EXPECT_EQ(qb::linq::from(empty).chunk(3).count(), 0u);
}

TEST(AuditChunk, SingleChunk)
{
    std::vector<int> data{1, 2, 3};
    auto chunks = qb::linq::from(data).chunk(5);
    EXPECT_EQ(chunks.count(), 1u);
    // Verify the chunk content by iterating directly
    auto it = chunks.begin();
    auto const& buf = *it;
    EXPECT_EQ(buf.size(), 3u);
    EXPECT_EQ(buf[0], 1);
    EXPECT_EQ(buf[1], 2);
    EXPECT_EQ(buf[2], 3);
}

TEST(AuditChunk, ExactMultiple)
{
    std::vector<int> data{1, 2, 3, 4, 5, 6};
    auto chunks = qb::linq::from(data).chunk(3);
    EXPECT_EQ(chunks.count(), 2u);
}

TEST(AuditChunk, UnevenLastChunk)
{
    std::vector<int> data{1, 2, 3, 4, 5};
    auto chunks = qb::linq::from(data).chunk(3);
    EXPECT_EQ(chunks.count(), 2u);
    // Last chunk should have 2 elements
    auto it = chunks.begin();
    ++it;
    EXPECT_EQ(qb::linq::from(*it).count(), 2u);
}

TEST(AuditChunk, ChunkOfOne)
{
    std::vector<int> data{1, 2, 3};
    auto chunks = qb::linq::from(data).chunk(1);
    EXPECT_EQ(chunks.count(), 3u);
}

// ---------------------------------------------------------------------------
// Stride comprehensive tests
// ---------------------------------------------------------------------------

TEST(AuditStride, EmptySequence)
{
    std::vector<int> empty;
    EXPECT_EQ(qb::linq::from(empty).stride(2).count(), 0u);
}

TEST(AuditStride, StepOne)
{
    std::vector<int> data{1, 2, 3, 4};
    auto result = qb::linq::from(data).stride(1).to_vector();
    std::vector<int> expected{1, 2, 3, 4};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(AuditStride, StepTwo)
{
    std::vector<int> data{1, 2, 3, 4, 5};
    auto result = qb::linq::from(data).stride(2).to_vector();
    std::vector<int> expected{1, 3, 5};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(AuditStride, StepLargerThanSequence)
{
    std::vector<int> data{1, 2, 3};
    auto result = qb::linq::from(data).stride(10).to_vector();
    EXPECT_EQ(result.count(), 1u);
    EXPECT_EQ(result.first(), 1);
}

TEST(AuditStride, SingleElement)
{
    std::vector<int> data{42};
    auto result = qb::linq::from(data).stride(3).to_vector();
    EXPECT_EQ(result.count(), 1u);
    EXPECT_EQ(result.first(), 42);
}

// ---------------------------------------------------------------------------
// Repeated enumeration
// ---------------------------------------------------------------------------

TEST(AuditRepeatedEnumeration, LazyViewCanBeIteratedTwice)
{
    std::vector<int> data{1, 2, 3, 4, 5};
    auto q = qb::linq::from(data).where([](int x) { return x % 2 != 0; });

    auto first_pass = q.to_vector();
    auto second_pass = q.to_vector();

    EXPECT_TRUE(first_pass.sequence_equal(second_pass));
}

TEST(AuditRepeatedEnumeration, DistinctViewRestartsOnNewPass)
{
    std::vector<int> data{1, 2, 2, 3, 3, 3};
    auto q = qb::linq::from(data).distinct();

    auto first_pass = q.to_vector();
    auto second_pass = q.to_vector();

    std::vector<int> expected{1, 2, 3};
    EXPECT_TRUE(first_pass.sequence_equal(expected));
    EXPECT_TRUE(second_pass.sequence_equal(expected));
}

TEST(AuditRepeatedEnumeration, ScanRestartsOnNewPass)
{
    std::vector<int> data{1, 2, 3};
    auto q = qb::linq::from(data).scan(0, [](int acc, int x) { return acc + x; });

    auto first_pass = q.to_vector();
    auto second_pass = q.to_vector();

    std::vector<int> expected{1, 3, 6};
    EXPECT_TRUE(first_pass.sequence_equal(expected));
    EXPECT_TRUE(second_pass.sequence_equal(expected));
}

// ---------------------------------------------------------------------------
// Laziness verification with side effects
// ---------------------------------------------------------------------------

TEST(AuditLaziness, SelectIsTrulyLazy)
{
    int call_count = 0;
    std::vector<int> data{1, 2, 3, 4, 5};
    auto q = qb::linq::from(data).select([&](int x) {
        ++call_count;
        return x * 2;
    });

    EXPECT_EQ(call_count, 0); // no calls yet

    (void)q.first(); // should trigger exactly 1 call
    EXPECT_EQ(call_count, 1);
}

TEST(AuditLaziness, WhereIsTrulyLazy)
{
    int call_count = 0;
    std::vector<int> data{1, 2, 3, 4, 5};
    auto q = qb::linq::from(data).where([&](int x) {
        ++call_count;
        return x > 3;
    });

    EXPECT_EQ(call_count, 0); // no calls until iteration

    auto it = q.begin();
    // where_view caches first match on first begin() - will scan until match
    EXPECT_GT(call_count, 0);
}

TEST(AuditLaziness, TakeShortCircuits)
{
    int call_count = 0;
    std::vector<int> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto result = qb::linq::from(data)
                      .select([&](int x) {
                          ++call_count;
                          return x;
                      })
                      .take(3)
                      .to_vector();

    EXPECT_EQ(result.count(), 3u);
    EXPECT_EQ(call_count, 3); // should not evaluate beyond first 3
}

// ---------------------------------------------------------------------------
// Edge cases: empty inputs for all operators
// ---------------------------------------------------------------------------

TEST(AuditEmpty, SelectOnEmpty)
{
    std::vector<int> empty;
    EXPECT_EQ(qb::linq::from(empty).select([](int x) { return x * 2; }).count(), 0u);
}

TEST(AuditEmpty, WhereOnEmpty)
{
    std::vector<int> empty;
    EXPECT_EQ(qb::linq::from(empty).where([](int) { return true; }).count(), 0u);
}

TEST(AuditEmpty, ConcatEmptyWithEmpty)
{
    std::vector<int> a, b;
    EXPECT_EQ(qb::linq::from(a).concat(b).count(), 0u);
}

TEST(AuditEmpty, ZipEmptyWithNonEmpty)
{
    std::vector<int> a;
    std::vector<int> b{1, 2, 3};
    EXPECT_EQ(qb::linq::from(a).zip(b).count(), 0u);
}

TEST(AuditEmpty, ScanOnEmpty)
{
    std::vector<int> empty;
    EXPECT_EQ(qb::linq::from(empty).scan(0, [](int a, int b) { return a + b; }).count(), 0u);
}

TEST(AuditEmpty, DistinctOnEmpty)
{
    std::vector<int> empty;
    EXPECT_EQ(qb::linq::from(empty).distinct().count(), 0u);
}

TEST(AuditEmpty, ReverseOnEmpty)
{
    std::vector<int> empty;
    EXPECT_EQ(qb::linq::from(empty).reverse().count(), 0u);
}

TEST(AuditEmpty, OrderByOnEmpty)
{
    std::vector<int> empty;
    EXPECT_EQ(qb::linq::from(empty).order_by(qb::linq::asc([](int x) { return x; })).count(), 0u);
}

TEST(AuditEmpty, GroupByOnEmpty)
{
    std::vector<int> empty;
    auto g = qb::linq::from(empty).group_by([](int x) { return x; });
    EXPECT_EQ(g.count(), 0u);
}

// ---------------------------------------------------------------------------
// Single-element edge cases
// ---------------------------------------------------------------------------

TEST(AuditSingleElement, MinMaxSameElement)
{
    std::vector<int> data{42};
    auto [lo, hi] = qb::linq::from(data).min_max();
    EXPECT_EQ(lo, 42);
    EXPECT_EQ(hi, 42);
}

TEST(AuditSingleElement, AverageSingleElement)
{
    std::vector<double> data{3.14};
    EXPECT_DOUBLE_EQ(qb::linq::from(data).average(), 3.14);
}

TEST(AuditSingleElement, OrderBySingleElement)
{
    std::vector<int> data{42};
    auto result = qb::linq::from(data).order_by(qb::linq::asc([](int x) { return x; }));
    EXPECT_EQ(result.first(), 42);
}

TEST(AuditSingleElement, DistinctSingleElement)
{
    std::vector<int> data{42};
    auto result = qb::linq::from(data).distinct().to_vector();
    EXPECT_EQ(result.count(), 1u);
    EXPECT_EQ(result.first(), 42);
}

TEST(AuditSingleElement, ChunkSingleElement)
{
    std::vector<int> data{42};
    auto chunks = qb::linq::from(data).chunk(5);
    EXPECT_EQ(chunks.count(), 1u);
}

// ---------------------------------------------------------------------------
// Join edge cases
// ---------------------------------------------------------------------------

TEST(AuditJoin, EmptyLeftJoinYieldsNoRows)
{
    std::vector<int> left;
    std::vector<int> right{1, 2, 3};
    auto result = qb::linq::from(left)
                      .join(right, [](int x) { return x; }, [](int x) { return x; },
                          [](int l, int r) { return l + r; });
    EXPECT_EQ(result.count(), 0u);
}

TEST(AuditJoin, EmptyRightJoinYieldsNoRows)
{
    std::vector<int> left{1, 2, 3};
    std::vector<int> right;
    auto result = qb::linq::from(left)
                      .join(right, [](int x) { return x; }, [](int x) { return x; },
                          [](int l, int r) { return l + r; });
    EXPECT_EQ(result.count(), 0u);
}

TEST(AuditJoin, MultiMatchInnerJoin)
{
    std::vector<int> left{1, 2, 2, 3};
    std::vector<int> right{2, 2, 4};
    auto result = qb::linq::from(left)
                      .join(right, [](int x) { return x; }, [](int x) { return x; },
                          [](int l, int r) { return l + r; });
    // 2 left matches × 2 right matches = 4 rows
    EXPECT_EQ(result.count(), 4u);
}

// ---------------------------------------------------------------------------
// Duplicate handling
// ---------------------------------------------------------------------------

TEST(AuditDuplicates, DistinctPreservesFirstSeen)
{
    std::vector<int> data{3, 1, 2, 1, 3, 2};
    auto result = qb::linq::from(data).distinct().to_vector();
    std::vector<int> expected{3, 1, 2};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(AuditDuplicates, ExceptPreservesFirstSeenOrder)
{
    std::vector<int> left{1, 2, 3, 2, 1};
    std::vector<int> right{2};
    auto result = qb::linq::from(left).except(right).to_vector();
    std::vector<int> expected{1, 3};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(AuditDuplicates, IntersectPreservesFirstSeenOrder)
{
    std::vector<int> left{3, 1, 2, 1, 3};
    std::vector<int> right{1, 3};
    auto result = qb::linq::from(left).intersect(right).to_vector();
    std::vector<int> expected{3, 1};
    EXPECT_TRUE(result.sequence_equal(expected));
}

// ---------------------------------------------------------------------------
// Factories
// ---------------------------------------------------------------------------

TEST(AuditFactories, EmptyFactory)
{
    auto e = qb::linq::empty<int>();
    EXPECT_EQ(e.count(), 0u);
    EXPECT_FALSE(e.any());
}

TEST(AuditFactories, OnceFactory)
{
    auto e = qb::linq::once(42);
    EXPECT_EQ(e.count(), 1u);
    EXPECT_EQ(e.first(), 42);
}

TEST(AuditFactories, RepeatFactory)
{
    auto e = qb::linq::repeat(7, 5);
    EXPECT_EQ(e.count(), 5u);
    EXPECT_TRUE(e.all_if([](int x) { return x == 7; }));
}

TEST(AuditFactories, IotaRange)
{
    auto e = qb::linq::iota(0, 5);
    EXPECT_EQ(e.count(), 5u);
    EXPECT_EQ(e.sum(), 10); // 0+1+2+3+4
}

TEST(AuditFactories, IotaEmptyRange)
{
    auto e = qb::linq::iota(5, 5);
    EXPECT_EQ(e.count(), 0u);
}

// ---------------------------------------------------------------------------
// Sequence equality edge cases
// ---------------------------------------------------------------------------

TEST(AuditSequenceEqual, BothEmpty)
{
    std::vector<int> a, b;
    EXPECT_TRUE(qb::linq::from(a).sequence_equal(b));
}

TEST(AuditSequenceEqual, DifferentLengths)
{
    std::vector<int> a{1, 2, 3};
    std::vector<int> b{1, 2};
    EXPECT_FALSE(qb::linq::from(a).sequence_equal(b));
}

TEST(AuditSequenceEqual, SameElements)
{
    std::vector<int> a{1, 2, 3};
    std::vector<int> b{1, 2, 3};
    EXPECT_TRUE(qb::linq::from(a).sequence_equal(b));
}

// ---------------------------------------------------------------------------
// Scan edge cases
// ---------------------------------------------------------------------------

TEST(AuditScan, SingleElement)
{
    std::vector<int> data{5};
    auto result = qb::linq::from(data).scan(0, [](int acc, int x) { return acc + x; }).to_vector();
    std::vector<int> expected{5};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(AuditScan, RunningProduct)
{
    std::vector<int> data{1, 2, 3, 4};
    auto result =
        qb::linq::from(data).scan(1, [](int acc, int x) { return acc * x; }).to_vector();
    std::vector<int> expected{1, 2, 6, 24};
    EXPECT_TRUE(result.sequence_equal(expected));
}

// ---------------------------------------------------------------------------
// Zip edge cases
// ---------------------------------------------------------------------------

TEST(AuditZip, DifferentLengthsStopsAtShorter)
{
    std::vector<int> a{1, 2, 3, 4, 5};
    std::vector<int> b{10, 20};
    auto result = qb::linq::from(a)
                      .zip(b)
                      .select([](auto const& p) { return p.first + p.second; })
                      .to_vector();
    std::vector<int> expected{11, 22};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(AuditZip, BothEmpty)
{
    std::vector<int> a, b;
    EXPECT_EQ(qb::linq::from(a).zip(b).count(), 0u);
}

TEST(AuditZip, ThreeWayZip)
{
    std::vector<int> a{1, 2};
    std::vector<int> b{10, 20};
    std::vector<int> c{100, 200};
    auto result = qb::linq::from(a)
                      .zip(b, c)
                      .select([](auto const& t) {
                          return std::get<0>(t) + std::get<1>(t) + std::get<2>(t);
                      })
                      .to_vector();
    std::vector<int> expected{111, 222};
    EXPECT_TRUE(result.sequence_equal(expected));
}

// ---------------------------------------------------------------------------
// Default if empty edge cases
// ---------------------------------------------------------------------------

TEST(AuditDefaultIfEmpty, NonEmptySequenceUnchanged)
{
    std::vector<int> data{1, 2, 3};
    auto result = qb::linq::from(data).default_if_empty(99).to_vector();
    std::vector<int> expected{1, 2, 3};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(AuditDefaultIfEmpty, EmptySequenceYieldsDefault)
{
    std::vector<int> empty;
    auto result = qb::linq::from(empty).default_if_empty(99).to_vector();
    EXPECT_EQ(result.count(), 1u);
    EXPECT_EQ(result.first(), 99);
}

// ---------------------------------------------------------------------------
// Aggregate edge cases
// ---------------------------------------------------------------------------

TEST(AuditAggregate, FoldOnSingleElement)
{
    std::vector<int> data{42};
    auto result = qb::linq::from(data).fold(0, [](int acc, int x) { return acc + x; });
    EXPECT_EQ(result, 42);
}

TEST(AuditAggregate, ReduceThrowsOnEmpty)
{
    std::vector<int> empty;
    EXPECT_THROW(
        qb::linq::from(empty).reduce([](int a, int b) { return a + b; }), std::out_of_range);
}

TEST(AuditAggregate, ReduceOnSingleReturnsElement)
{
    std::vector<int> data{42};
    auto result = qb::linq::from(data).reduce([](int a, int b) { return a + b; });
    EXPECT_EQ(result, 42);
}

// ---------------------------------------------------------------------------
// Exception correctness
// ---------------------------------------------------------------------------

TEST(AuditExceptions, FirstOnEmptyThrows)
{
    std::vector<int> empty;
    EXPECT_THROW(qb::linq::from(empty).first(), std::out_of_range);
}

TEST(AuditExceptions, SingleOnEmptyThrows)
{
    std::vector<int> empty;
    EXPECT_THROW((void)qb::linq::from(empty).single(), std::out_of_range);
}

TEST(AuditExceptions, SingleOnMultipleThrows)
{
    std::vector<int> data{1, 2};
    EXPECT_THROW((void)qb::linq::from(data).single(), std::out_of_range);
}

TEST(AuditExceptions, ElementAtOutOfBoundsThrows)
{
    std::vector<int> data{1, 2, 3};
    EXPECT_THROW((void)qb::linq::from(data).element_at(5), std::out_of_range);
}

TEST(AuditExceptions, ToDictionaryDuplicateKeyThrows)
{
    std::vector<int> data{1, 2, 1};
    EXPECT_THROW(qb::linq::from(data).to_dictionary(
                     [](int x) { return x; }, [](int x) { return x; }),
        std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Zip fold
// ---------------------------------------------------------------------------

TEST(AuditZipFold, DotProduct)
{
    std::vector<int> a{1, 2, 3};
    std::vector<int> b{4, 5, 6};
    auto result = qb::linq::from(a).zip_fold(b, 0, [](int acc, int x, int y) { return acc + x * y; });
    EXPECT_EQ(result, 32); // 1*4 + 2*5 + 3*6
}

TEST(AuditZipFold, EmptyInputs)
{
    std::vector<int> a;
    std::vector<int> b{1, 2, 3};
    auto result = qb::linq::from(a).zip_fold(b, 0, [](int acc, int x, int y) { return acc + x + y; });
    EXPECT_EQ(result, 0);
}

// ---------------------------------------------------------------------------
// forward_list (strictly forward iterators)
// ---------------------------------------------------------------------------

TEST(AuditForwardList, SelectAndWhere)
{
    std::forward_list<int> data{1, 2, 3, 4, 5};
    auto result = qb::linq::from(data)
                      .where([](int x) { return x % 2 == 0; })
                      .select([](int x) { return x * 10; })
                      .to_vector();
    std::vector<int> expected{20, 40};
    EXPECT_TRUE(result.sequence_equal(expected));
}

TEST(AuditForwardList, CountAndSum)
{
    std::forward_list<int> data{1, 2, 3};
    EXPECT_EQ(qb::linq::from(data).count(), 3u);
    EXPECT_EQ(qb::linq::from(data).sum(), 6);
}
