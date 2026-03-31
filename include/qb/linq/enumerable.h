#pragma once

/**
 * @file enumerable.h
 * @ingroup linq
 * @brief `enumerable<Handle>` — public fluent wrapper; factories `from`, `as_enumerable`, `iota`, `empty`, …
 *
 * @details
 * `enumerable` inherits privately from `Handle` (a `query_state`, `where_view`, `materialized_range`, etc.)
 * and forwards every query method to the handle. Lazy operations wrap the result in a new `enumerable` via
 * `pipe()`. Terminals return values or materialized ranges directly. See `@ref linq` for caveats
 * (`select_many`, `zip`/`zip_fold`/`concat` copyability, thread safety).
 */

#include <cstddef>
#include <type_traits>
#include <utility>

#include "qb/linq/query.h"

/**
 * @namespace qb::linq
 * @ingroup linq
 * @brief Root namespace: `enumerable`, factories (`from`, `iota`, …), re-exports. Range handles and iterators live in
 *        `query.h` / `iterators.h` / `detail/` (same namespace).
 */
namespace qb::linq {

/**
 * @ingroup linq
 * @brief Fluent LINQ-style façade over a pipeline handle (`Handle` models a range + algorithms).
 *
 * @tparam Handle Concrete view or materialized range type (must expose `value_type`, `begin`, `end`, and
 *         algorithms from `query_range_algorithms` or `materialized_range` extras).
 *
 * @par Lazy operations
 * `select`, `select_many`, `where`, `of_type`, `skip`, `skip_while`, `take`, `take_while`, `skip_last`,
 * `take_last`, `reverse`, `concat`, `zip`, `default_if_empty`, `enumerate`, `scan`, `chunk`, `stride`,
 * `distinct`, `distinct_by`, `union_with`, `join`, `group_join`, `to_lookup`, `group_by`, `order_by`,
 * `except`, `intersect`, map/set materializers, `append`, `prepend`.
 *
 * @par Terminals (eager)
 * `first`/`last`/`single` families, `element_at`, `contains`, `index_of`, `sequence_equal`, `count`,
 * `any`/`all_if`/`none_if`, `sum`/`average`/min/max/`min_max`/`min_by`/`max_by`, `aggregate`/`fold`/`reduce`,
 * percentiles, variance, `std_dev`, `each`, `to_vector`/`materialize`, `operator[]` when supported.
 *
 * @par Related
 * `from`, `range`, `as_enumerable`, `iota`, `empty`, `once`, `repeat`, `asc`/`desc`/`make_filter`, CTAD
 * `enumerable(h)`.
 *
 * @par Source of truth (Doxygen)
 * Full semantics, complexity, preconditions, and exception messages are on `qb::linq::detail::query_range_algorithms`
 * and concrete handles (`where_view`, `materialized_range`, …). Members below **forward** to `Handle` with the same
 * signatures; one-line `@brief` tags name the LINQ operation and note pipe vs terminal where helpful.
 *
 * @par Return categories
 * - **`auto` + `pipe(...)`:** new lazy `enumerable` stage (or `materialized_range` after materializers).
 * - **`decltype(auto)`:** often a **reference** into the sequence — do not use after invalidation (see `@ref linq` caveats).
 * - **`double` / `bool` / `std::size_t`:** eager scalars.
 */
template <class Handle>
class enumerable : private Handle {
    /** @brief Wraps the next pipeline stage in `enumerable<std::decay_t<Next>>`. */
    template <class Next>
    [[nodiscard]] static enumerable<std::decay_t<Next>> pipe(Next&& next)
    {
        return enumerable<std::decay_t<Next>>(std::forward<Next>(next));
    }

public:
    /** @name Construction */
    /** @{ */
    /** @brief Inherit `Handle` constructors when applicable (same as underlying pipeline type). */
    using Handle::Handle;
    /** @brief Same as `iterator_traits::value_type` of the handle’s iterator (const-stripped reference target). */
    using value_type = typename Handle::value_type;

    /** @brief Takes ownership of a completed handle (view or materialized range). */
    explicit enumerable(Handle h) : Handle(std::move(h)) {}

    /** @brief Copy semantics: copies the embedded `Handle`. */
    enumerable(enumerable const&) = default;
    /** @brief Copy assignment. */
    enumerable& operator=(enumerable const&) = default;
    /** @brief Move construction. */
    enumerable(enumerable&&) = default;
    /** @brief Move assignment. */
    enumerable& operator=(enumerable&&) = default;
    /** @} */

    /** @name Iteration (begin / end) */
    /** @{ */
    using Handle::begin;
    using Handle::end;
    /** @} */

    /** @name Element access (terminals) */
    /** @{ */
    /** @brief First element (LINQ `First`); throws if empty. */
    [[nodiscard]] decltype(auto) first() const { return static_cast<Handle const&>(*this).first(); }

    /** @brief First element or default-constructed `value_type` if empty (LINQ `FirstOrDefault`). */
    [[nodiscard]] decltype(auto) first_or_default() const
    {
        return static_cast<Handle const&>(*this).first_or_default();
    }

    /** @brief First element matching `pred` (LINQ `First` with predicate). */
    template <class Pred>
    [[nodiscard]] decltype(auto) first_if(Pred&& pred) const
    {
        return static_cast<Handle const&>(*this).first_if(std::forward<Pred>(pred));
    }

    /** @brief First match by predicate or default if none (LINQ `FirstOrDefault` + predicate). */
    template <class Pred>
    [[nodiscard]] decltype(auto) first_or_default_if(Pred&& pred) const
    {
        return static_cast<Handle const&>(*this).first_or_default_if(std::forward<Pred>(pred));
    }

    /** @brief Last element (LINQ `Last`); throws if empty. */
    [[nodiscard]] decltype(auto) last() const { return static_cast<Handle const&>(*this).last(); }

    /** @brief Last element or default if empty (LINQ `LastOrDefault`). */
    [[nodiscard]] decltype(auto) last_or_default() const
    {
        return static_cast<Handle const&>(*this).last_or_default();
    }

    /** @brief Last element matching `pred` (full scan). */
    template <class Pred>
    [[nodiscard]] decltype(auto) last_if(Pred&& pred) const
    {
        return static_cast<Handle const&>(*this).last_if(std::forward<Pred>(pred));
    }

    /** @brief Last match by predicate or default if none. */
    template <class Pred>
    [[nodiscard]] decltype(auto) last_or_default_if(Pred&& pred) const
    {
        return static_cast<Handle const&>(*this).last_or_default_if(std::forward<Pred>(pred));
    }

    /** @brief Zero-based element by index (LINQ `ElementAt`). */
    [[nodiscard]] decltype(auto) element_at(std::size_t i) const
    {
        return static_cast<Handle const&>(*this).element_at(i);
    }

    /** @brief Indexed access or default if out of range (LINQ `ElementAtOrDefault`). */
    [[nodiscard]] decltype(auto) element_at_or_default(std::size_t i) const
    {
        return static_cast<Handle const&>(*this).element_at_or_default(i);
    }

    /** @brief Same as `count()` (LINQ `LongCount`). */
    [[nodiscard]] std::size_t long_count() const
    {
        return static_cast<Handle const&>(*this).long_count();
    }

    /** @brief Count of elements satisfying `pred` (LINQ `LongCount` + predicate). */
    template <class Pred>
    [[nodiscard]] std::size_t long_count_if(Pred&& pred) const
    {
        return static_cast<Handle const&>(*this).long_count_if(std::forward<Pred>(pred));
    }
    /** @} */

    /** @name Sequence comparison */
    /** @{ */
    /** @brief Pairwise `==` until one range ends (LINQ `SequenceEqual`). */
    template <class Rng>
    [[nodiscard]] bool sequence_equal(Rng const& rhs) const
    {
        return static_cast<Handle const&>(*this).sequence_equal(rhs);
    }

    /** @brief Pairwise comparison with `comp` (LINQ `SequenceEqual` + comparer). */
    template <class Rng, class Comp>
    [[nodiscard]] bool sequence_equal(Rng const& rhs, Comp&& comp) const
    {
        return static_cast<Handle const&>(*this).sequence_equal(rhs, std::forward<Comp>(comp));
    }
    /** @} */

    /** @name Lazy composition */
    /** @{ */
    /** @brief Concatenate this sequence with `rhs` (same `value_type`; LINQ `Concat`). */
    template <class Rng>
    [[nodiscard]] auto concat(Rng const& rhs) const
    {
        return pipe(static_cast<Handle const&>(*this).concat(rhs));
    }

    /** @brief Pairwise zip with `rhs` (stops at shorter range; LINQ `Zip`). */
    template <class Rng>
    [[nodiscard]] auto zip(Rng const& rhs) const
    {
        return pipe(static_cast<Handle const&>(*this).zip(rhs));
    }

    /** @brief Single-pass fold over pairs with `rhs` (shorter length wins); prefer over `zip().select(...).aggregate(...)`. */
    template <class Rng, class Acc, class F>
    [[nodiscard]] std::decay_t<Acc> zip_fold(Rng const& rhs, Acc&& init, F&& f) const
    {
        return static_cast<Handle const&>(*this).zip_fold(
            rhs, std::forward<Acc>(init), std::forward<F>(f));
    }

    /** @brief Yield `def` once if empty; otherwise forward source (LINQ `DefaultIfEmpty`). */
    [[nodiscard]] auto default_if_empty(value_type def) const
    {
        return pipe(static_cast<Handle const&>(*this).default_if_empty(std::move(def)));
    }

    /** @brief `DefaultIfEmpty` with default-constructed `value_type`. */
    [[nodiscard]] auto default_if_empty() const
    {
        return pipe(static_cast<Handle const&>(*this).default_if_empty());
    }

    /** @brief `(index, element)` pairs from 0 (LINQ indexed `Select`). */
    [[nodiscard]] auto enumerate() const
    {
        return pipe(static_cast<Handle const&>(*this).enumerate());
    }

    /** @brief Inclusive prefix scan with seed `seed` and binary `f` (LINQ `Scan`). */
    template <class Acc, class F>
    [[nodiscard]] auto scan(Acc&& seed, F&& f) const
    {
        return pipe(static_cast<Handle const&>(*this).scan(
            std::forward<Acc>(seed), std::forward<F>(f)));
    }
    /** @} */

    /** @name Joins and lookups */
    /** @{ */
    /** @brief Inner join on keys; `result(outer, inner)` builds each row (LINQ `Join`). */
    template <class Rng, class OuterKey, class InnerKey, class ResultSel>
    [[nodiscard]] auto join(
        Rng const& inner, OuterKey&& ok, InnerKey&& ik, ResultSel&& rs) const
    {
        return pipe(static_cast<Handle const&>(*this).join(
            inner, std::forward<OuterKey>(ok), std::forward<InnerKey>(ik), std::forward<ResultSel>(rs)));
    }

    /** @brief Group outer with inner matches per key (LINQ `GroupJoin`). */
    template <class Rng, class OuterKey, class InnerKey>
    [[nodiscard]] auto group_join(Rng const& inner, OuterKey&& ok, InnerKey&& ik) const
    {
        return pipe(static_cast<Handle const&>(*this).group_join(
            inner, std::forward<OuterKey>(ok), std::forward<InnerKey>(ik)));
    }

    /** @brief Same as `group_by(key)` (LINQ `ToLookup`). */
    template <class KeyFn>
    [[nodiscard]] auto to_lookup(KeyFn&& key) const
    {
        return pipe(static_cast<Handle const&>(*this).to_lookup(std::forward<KeyFn>(key)));
    }
    /** @} */

    /** @name Chunking, stride, append */
    /** @{ */
    /** @brief Contiguous chunks of up to `size` elements (each chunk is a `std::vector` by value). */
    [[nodiscard]] auto chunk(std::size_t size) const
    {
        return pipe(static_cast<Handle const&>(*this).chunk(size));
    }

    /** @brief Every `step`-th element (`step < 1` clamped to 1). */
    [[nodiscard]] auto stride(std::size_t step) const
    {
        return pipe(static_cast<Handle const&>(*this).stride(step));
    }

    /** @brief Append one element after this sequence (LINQ `Append`). */
    template <class T>
    [[nodiscard]] auto append(T&& v) const
    {
        return pipe(static_cast<Handle const&>(*this).append(std::forward<T>(v)));
    }

    /** @brief Prepend one element before this sequence (LINQ `Prepend`). */
    template <class T>
    [[nodiscard]] auto prepend(T&& v) const
    {
        return pipe(static_cast<Handle const&>(*this).prepend(std::forward<T>(v)));
    }
    /** @} */

    /** @name Sets and maps (materializing) */
    /** @{ */
    /** @brief Unique elements in unspecified order (`std::unordered_set`). */
    [[nodiscard]] auto to_unordered_set() const
    {
        return pipe(static_cast<Handle const&>(*this).to_unordered_set());
    }

    /** @brief Unique elements in sorted order (`std::set`). */
    [[nodiscard]] auto to_set() const { return pipe(static_cast<Handle const&>(*this).to_set()); }

    /** @brief Element with minimum `key(*it)`; throws if empty. */
    template <class KF>
    [[nodiscard]] decltype(auto) min_by(KF&& key) const
    {
        return static_cast<Handle const&>(*this).min_by(std::forward<KF>(key));
    }

    /** @brief Element with maximum `key(*it)`; throws if empty. */
    template <class KF>
    [[nodiscard]] decltype(auto) max_by(KF&& key) const
    {
        return static_cast<Handle const&>(*this).max_by(std::forward<KF>(key));
    }

    /** @brief Distinct by key function (LINQ `DistinctBy`). */
    template <class KF>
    [[nodiscard]] auto distinct_by(KF&& kf) const
    {
        return pipe(static_cast<Handle const&>(*this).distinct_by(std::forward<KF>(kf)));
    }

    /** @brief Distinct elements by value (LINQ `Distinct`). */
    [[nodiscard]] auto distinct() const { return pipe(static_cast<Handle const&>(*this).distinct()); }

    /** @brief Multiset union then distinct (LINQ `Union`). */
    template <class Rng>
    [[nodiscard]] auto union_with(Rng const& rhs) const
    {
        return pipe(static_cast<Handle const&>(*this).union_with(rhs));
    }

    /** @brief `unordered_map` from key and element functions (last wins on duplicate key). */
    template <class KeyFn, class ElemFn>
    [[nodiscard]] auto to_unordered_map(KeyFn&& keyf, ElemFn&& elemf) const
    {
        return pipe(static_cast<Handle const&>(*this).to_unordered_map(
            std::forward<KeyFn>(keyf), std::forward<ElemFn>(elemf)));
    }

    /** @brief Ordered `std::map` from key and element functions. */
    template <class KeyFn, class ElemFn>
    [[nodiscard]] auto to_map(KeyFn&& keyf, ElemFn&& elemf) const
    {
        return pipe(static_cast<Handle const&>(*this).to_map(
            std::forward<KeyFn>(keyf), std::forward<ElemFn>(elemf)));
    }

    /** @brief `unordered_map`; throws on duplicate key (LINQ `ToDictionary`). */
    template <class KeyFn, class ElemFn>
    [[nodiscard]] auto to_dictionary(KeyFn&& keyf, ElemFn&& elemf) const
    {
        return pipe(static_cast<Handle const&>(*this).to_dictionary(
            std::forward<KeyFn>(keyf), std::forward<ElemFn>(elemf)));
    }

    /** @brief Set difference using hash set of `rhs` (LINQ `Except`). */
    template <class Rng>
    [[nodiscard]] auto except(Rng const& rhs) const
    {
        return pipe(static_cast<Handle const&>(*this).except(rhs));
    }

    /** @brief Intersection with `rhs` (LINQ `Intersect`). */
    template <class Rng>
    [[nodiscard]] auto intersect(Rng const& rhs) const
    {
        return pipe(static_cast<Handle const&>(*this).intersect(rhs));
    }
    /** @} */

    /** @name Tail / slice */
    /** @{ */
    /** @brief Last `n` elements (materialized; ring buffer). */
    [[nodiscard]] auto take_last(std::size_t n) const
    {
        return pipe(static_cast<Handle const&>(*this).take_last(n));
    }

    /** @brief Drop last `n` elements (materialized). */
    [[nodiscard]] auto skip_last(std::size_t n) const
    {
        return pipe(static_cast<Handle const&>(*this).skip_last(n));
    }

    /** @brief Reverse iteration (bidirectional source; LINQ `Reverse`). */
    [[nodiscard]] auto reverse() const
    {
        return pipe(static_cast<Handle const&>(*this).reverse());
    }
    /** @} */

    /** @name Transform and filter */
    /** @{ */
    /** @brief Project each element with `f` (LINQ `Select`). */
    template <class F>
    [[nodiscard]] auto select(F&& f) const
    {
        return pipe(static_cast<Handle const&>(*this).select(std::forward<F>(f)));
    }

    /** @brief Tuple of projections per element — not C# `SelectMany` flattening (see `@ref linq`). */
    template <class... Fs>
    [[nodiscard]] auto select_many(Fs&&... fs) const
    {
        return pipe(static_cast<Handle const&>(*this).select_many(std::forward<Fs>(fs)...));
    }

    /** @brief Filter by predicate (LINQ `Where`). */
    template <class P>
    [[nodiscard]] auto where(P&& p) const
    {
        return pipe(static_cast<Handle const&>(*this).where(std::forward<P>(p)));
    }

    /** @brief `dynamic_cast` filter for polymorphic pointer sequences (LINQ `OfType`). */
    template <class U>
    [[nodiscard]] auto of_type() const
    {
        return pipe(static_cast<Handle const&>(*this).template of_type<U>());
    }

    /** @brief Nested grouping by one or more key functions (LINQ `GroupBy`). */
    template <class... Keys>
    [[nodiscard]] auto group_by(Keys&&... keys) const
    {
        return pipe(static_cast<Handle const&>(*this).group_by(std::forward<Keys>(keys)...));
    }

    /** @brief Stable sort copy with lexicographic key filters (LINQ `OrderBy`). */
    template <class... Kf>
    [[nodiscard]] auto order_by(Kf&&... kf) const
    {
        return pipe(static_cast<Handle const&>(*this).order_by(std::forward<Kf>(kf)...));
    }

    /** @brief Materialized ascending view (when `Handle` exposes `asc()`). */
    template <class H = Handle, class = std::void_t<decltype(std::declval<H const&>().asc())>>
    [[nodiscard]] auto asc() const
    {
        return pipe(static_cast<Handle const&>(*this).asc());
    }

    /** @brief Materialized descending view (when `Handle` exposes `desc()`). */
    template <class H = Handle, class = std::void_t<decltype(std::declval<H const&>().desc())>>
    [[nodiscard]] auto desc() const
    {
        return pipe(static_cast<Handle const&>(*this).desc());
    }

    /** @brief Skip first `n` elements (LINQ `Skip`). */
    [[nodiscard]] auto skip(std::size_t n) const { return pipe(static_cast<Handle const&>(*this).skip(n)); }

    /** @brief Skip leading prefix while `pred` holds (LINQ `SkipWhile`). */
    template <class P>
    [[nodiscard]] auto skip_while(P&& p) const
    {
        return pipe(static_cast<Handle const&>(*this).skip_while(std::forward<P>(p)));
    }

    /** @brief `take` first `n` elements (sign normalized; LINQ `Take`). */
    [[nodiscard]] auto take(int n) const { return pipe(static_cast<Handle const&>(*this).take(n)); }

    /** @brief Elements from start while `pred` holds (LINQ `TakeWhile`). */
    template <class P>
    [[nodiscard]] auto take_while(P&& p) const
    {
        return pipe(static_cast<Handle const&>(*this).take_while(std::forward<P>(p)));
    }
    /** @} */

    /** @name Side effects */
    /** @{ */
    /**
     * @brief Invokes `f` on each element, then returns a copy of this pipeline stage (safe for chaining).
     * @details Does not consume into a new owning range; further operations re-walk the sequence.
     */
    template <class F>
    [[nodiscard]] enumerable each(F&& f) const&
    {
        static_cast<Handle const&>(*this).each(std::forward<F>(f));
        return enumerable(static_cast<Handle const&>(*this));
    }

    /** @brief Non-`const` `enumerable&` overload of `each`. */
    template <class F>
    [[nodiscard]] enumerable each(F&& f) &
    {
        static_cast<Handle const&>(*this).each(std::forward<F>(f));
        return enumerable(static_cast<Handle const&>(*this));
    }
    /** @} */

    /** @name Search */
    /** @{ */
    /** @brief Whether `v` appears (LINQ `Contains`). */
    template <class T>
    [[nodiscard]] bool contains(T const& v) const
    {
        return static_cast<Handle const&>(*this).contains(v);
    }

    /** @brief `Contains` with custom equivalence `eq`. */
    template <class T, class Eq>
    [[nodiscard]] bool contains(T const& v, Eq&& eq) const
    {
        return static_cast<Handle const&>(*this).contains(v, std::forward<Eq>(eq));
    }

    /** @brief First index where `*it == v`, or `nullopt`. */
    template <class T>
    [[nodiscard]] auto index_of(T const& v) const
    {
        return static_cast<Handle const&>(*this).index_of(v);
    }

    /** @brief `index_of` with custom equivalence. */
    template <class T, class Eq>
    [[nodiscard]] auto index_of(T const& v, Eq&& eq) const
    {
        return static_cast<Handle const&>(*this).index_of(v, std::forward<Eq>(eq));
    }

    /** @brief Last index with `*it == v`, or `nullopt`. */
    template <class T>
    [[nodiscard]] auto last_index_of(T const& v) const
    {
        return static_cast<Handle const&>(*this).last_index_of(v);
    }

    /** @brief `last_index_of` with custom equivalence. */
    template <class T, class Eq>
    [[nodiscard]] auto last_index_of(T const& v, Eq&& eq) const
    {
        return static_cast<Handle const&>(*this).last_index_of(v, std::forward<Eq>(eq));
    }
    /** @} */

    /** @name Cardinality */
    /** @{ */
    /** @brief Non-empty sequence (LINQ `Any` without predicate). */
    [[nodiscard]] bool any() const noexcept { return static_cast<Handle const&>(*this).any(); }

    /** @brief Element count (`std::distance`; LINQ `Count`). */
    [[nodiscard]] std::size_t count() const { return static_cast<Handle const&>(*this).count(); }
    /** @} */

    /** @name Materialization */
    /** @{ */
    /** @brief Copy to `std::vector` (LINQ `ToArray`-style). */
    [[nodiscard]] auto to_vector() const { return pipe(static_cast<Handle const&>(*this).to_vector()); }

    /** @brief Same as `to_vector()` (LINQ `ToList`). */
    [[nodiscard]] auto materialize() const { return to_vector(); }
    /** @} */

    /** @name Min / max */
    /** @{ */
    /** @brief Minimum by `operator<`; throws if empty. */
    [[nodiscard]] decltype(auto) min() const { return static_cast<Handle const&>(*this).min(); }

    /** @brief Maximum by `operator<`; throws if empty. */
    [[nodiscard]] decltype(auto) max() const { return static_cast<Handle const&>(*this).max(); }

    /** @brief Minimum by comparator `comp`. */
    template <class Comp>
    [[nodiscard]] decltype(auto) min(Comp&& comp) const
    {
        return static_cast<Handle const&>(*this).min(std::forward<Comp>(comp));
    }

    /** @brief Maximum by comparator `comp`. */
    template <class Comp>
    [[nodiscard]] decltype(auto) max(Comp&& comp) const
    {
        return static_cast<Handle const&>(*this).max(std::forward<Comp>(comp));
    }

    /** @brief `(min, max)` in one pass by `operator<`. */
    [[nodiscard]] decltype(auto) min_max() const
    {
        return static_cast<Handle const&>(*this).min_max();
    }

    /** @brief `(min, max)` by `comp`. */
    template <class Comp>
    [[nodiscard]] decltype(auto) min_max(Comp&& comp) const
    {
        return static_cast<Handle const&>(*this).min_max(std::forward<Comp>(comp));
    }

    /** @brief Minimum or default if empty. */
    [[nodiscard]] decltype(auto) min_or_default() const
    {
        return static_cast<Handle const&>(*this).min_or_default();
    }

    /** @brief Maximum or default if empty. */
    [[nodiscard]] decltype(auto) max_or_default() const
    {
        return static_cast<Handle const&>(*this).max_or_default();
    }

    /** @brief `min_by` or default if empty. */
    template <class KF>
    [[nodiscard]] decltype(auto) min_by_or_default(KF&& key) const
    {
        return static_cast<Handle const&>(*this).min_by_or_default(std::forward<KF>(key));
    }

    /** @brief `max_by` or default if empty. */
    template <class KF>
    [[nodiscard]] decltype(auto) max_by_or_default(KF&& key) const
    {
        return static_cast<Handle const&>(*this).max_by_or_default(std::forward<KF>(key));
    }
    /** @} */

    /** @name Sum and aggregates */
    /** @{ */
    /** @brief Sum of elements (`+=`; LINQ `Sum`). */
    [[nodiscard]] decltype(auto) sum() const { return static_cast<Handle const&>(*this).sum(); }

    /** @brief Fused filter + sum (no `where` iterator overhead). */
    template <class Pred>
    [[nodiscard]] decltype(auto) sum_if(Pred&& pred) const
    {
        return static_cast<Handle const&>(*this).sum_if(std::forward<Pred>(pred));
    }

    /** @brief Count of elements matching `pred` (LINQ `Count` with predicate). */
    template <class Pred>
    [[nodiscard]] std::size_t count_if(Pred&& pred) const
    {
        return static_cast<Handle const&>(*this).count_if(std::forward<Pred>(pred));
    }

    /** @brief Any element matches `pred` (LINQ `Any` with predicate). */
    template <class Pred>
    [[nodiscard]] bool any_if(Pred&& pred) const
    {
        return static_cast<Handle const&>(*this).any_if(std::forward<Pred>(pred));
    }

    /** @brief All elements match `pred` (LINQ `All`). */
    template <class Pred>
    [[nodiscard]] bool all_if(Pred&& pred) const
    {
        return static_cast<Handle const&>(*this).all_if(std::forward<Pred>(pred));
    }

    /** @brief No element matches `pred` (LINQ `All` negated). */
    template <class Pred>
    [[nodiscard]] bool none_if(Pred&& pred) const
    {
        return static_cast<Handle const&>(*this).none_if(std::forward<Pred>(pred));
    }

    /** @brief Left fold with seed (LINQ `Aggregate` with seed). */
    template <class Acc, class F>
    [[nodiscard]] decltype(auto) aggregate(Acc&& init, F&& f) const
    {
        return static_cast<Handle const&>(*this).aggregate(std::forward<Acc>(init), std::forward<F>(f));
    }

    /** @brief Alias of `aggregate`. */
    template <class Acc, class F>
    [[nodiscard]] decltype(auto) fold(Acc&& init, F&& f) const
    {
        return static_cast<Handle const&>(*this).fold(std::forward<Acc>(init), std::forward<F>(f));
    }

    /** @brief Binary fold without seed; first element is initial `acc` (LINQ `Aggregate` without seed). */
    template <class F>
    [[nodiscard]] decltype(auto) reduce(F&& f) const
    {
        return static_cast<Handle const&>(*this).reduce(std::forward<F>(f));
    }
    /** @} */

    /** @name Averages and percentiles */
    /** @{ */
    /** @brief Arithmetic mean; throws if empty. */
    [[nodiscard]] double average() const { return static_cast<Handle const&>(*this).average(); }

    /** @brief Mean over elements matching `pred`. */
    template <class Pred>
    [[nodiscard]] double average_if(Pred&& pred) const
    {
        return static_cast<Handle const&>(*this).average_if(std::forward<Pred>(pred));
    }

    /** @brief Empirical percentile of `key(*it)` (`p` in `[0,100]`). */
    template <class KeyFn>
    [[nodiscard]] double percentile_by(double p, KeyFn&& key) const
    {
        return static_cast<Handle const&>(*this).percentile_by(p, std::forward<KeyFn>(key));
    }

    /** @brief Percentile of elements (arithmetic `value_type` only). */
    [[nodiscard]] double percentile(double p) const
    {
        return static_cast<Handle const&>(*this).percentile(p);
    }

    /** @brief Median of `key(*it)`. */
    template <class KeyFn>
    [[nodiscard]] double median_by(KeyFn&& key) const
    {
        return static_cast<Handle const&>(*this).median_by(std::forward<KeyFn>(key));
    }

    /** @brief Median of elements (arithmetic `value_type` only). */
    [[nodiscard]] double median() const { return static_cast<Handle const&>(*this).median(); }
    /** @} */

    /** @name Variance and standard deviation */
    /** @{ */
    /** @brief Population variance of `key(*it)` (Welford). */
    template <class KeyFn>
    [[nodiscard]] double variance_population_by(KeyFn&& key) const
    {
        return static_cast<Handle const&>(*this).variance_population_by(std::forward<KeyFn>(key));
    }

    /** @brief Sample variance of `key(*it)` (Welford). */
    template <class KeyFn>
    [[nodiscard]] double variance_sample_by(KeyFn&& key) const
    {
        return static_cast<Handle const&>(*this).variance_sample_by(std::forward<KeyFn>(key));
    }

    /** @brief Population variance of elements. */
    [[nodiscard]] double variance_population() const
    {
        return static_cast<Handle const&>(*this).variance_population();
    }

    /** @brief Sample variance of elements. */
    [[nodiscard]] double variance_sample() const
    {
        return static_cast<Handle const&>(*this).variance_sample();
    }

    /** @brief Population standard deviation of `key(*it)`. */
    template <class KeyFn>
    [[nodiscard]] double std_dev_population_by(KeyFn&& key) const
    {
        return static_cast<Handle const&>(*this).std_dev_population_by(std::forward<KeyFn>(key));
    }

    /** @brief Sample standard deviation of `key(*it)`. */
    template <class KeyFn>
    [[nodiscard]] double std_dev_sample_by(KeyFn&& key) const
    {
        return static_cast<Handle const&>(*this).std_dev_sample_by(std::forward<KeyFn>(key));
    }

    /** @brief Population standard deviation of elements. */
    [[nodiscard]] double std_dev_population() const
    {
        return static_cast<Handle const&>(*this).std_dev_population();
    }

    /** @brief Sample standard deviation of elements. */
    [[nodiscard]] double std_dev_sample() const
    {
        return static_cast<Handle const&>(*this).std_dev_sample();
    }
    /** @} */

    /** @name Single element */
    /** @{ */
    /** @brief Exactly one element; throws if length ≠ 1 (LINQ `Single`). */
    [[nodiscard]] decltype(auto) single() const { return static_cast<Handle const&>(*this).single(); }

    /** @brief One element or default if empty or length > 1. */
    [[nodiscard]] decltype(auto) single_or_default() const
    {
        return static_cast<Handle const&>(*this).single_or_default();
    }

    /** @brief Unique element matching `pred`; throws if 0 or >1 matches. */
    template <class Pred>
    [[nodiscard]] decltype(auto) single_if(Pred&& pred) const
    {
        return static_cast<Handle const&>(*this).single_if(std::forward<Pred>(pred));
    }

    /** @brief At most one match: value or default; default if ambiguous. */
    template <class Pred>
    [[nodiscard]] decltype(auto) single_or_default_if(Pred&& pred) const
    {
        return static_cast<Handle const&>(*this).single_or_default_if(std::forward<Pred>(pred));
    }
    /** @} */

    /** @name Keyed access */
    /** @{ */
    /** @brief Random access by key when `Handle` supports it (e.g. `materialized_range` over a map). */
    template <class K, class H = Handle,
        class = std::void_t<decltype(std::declval<H const&>()[std::declval<K const&>()])>>
    [[nodiscard]] decltype(auto) operator[](K const& key) const
    {
        return static_cast<Handle const&>(*this)[key];
    }
    /** @} */
};

/** @name Free functions (factories and helpers) */
/** @{ */

/** @brief Identity: returns the same `enumerable` lvalue (LINQ `AsEnumerable`). */
template <class H>
[[nodiscard]] enumerable<H> as_enumerable(enumerable<H>& e)
{
    return e;
}

/** @brief `const` overload of `as_enumerable` for `enumerable`. */
template <class H>
[[nodiscard]] enumerable<H> as_enumerable(enumerable<H> const& e)
{
    return e;
}

/** @brief Rvalue overload: moves the pipeline handle out of the temporary `enumerable`. */
template <class H>
[[nodiscard]] enumerable<H> as_enumerable(enumerable<H>&& e)
{
    return std::move(e);
}

/** @brief Wraps a mutable container as an `enumerable` (`from`). */
template <class C>
[[nodiscard]] auto as_enumerable(C& container)
{
    return from(container);
}

/** @brief Wraps a const container as an `enumerable` (const iterators). */
template <class C>
[[nodiscard]] auto as_enumerable(C const& container)
{
    return from(container);
}

/**
 * @brief Iterator-pair entry: same as `from(b, e)`.
 * @param b Half-open begin.
 * @param e Half-open end.
 */
template <class Iter>
[[nodiscard]] auto as_enumerable(Iter b, Iter e)
{
    return from(std::move(b), std::move(e));
}

/**
 * @brief Starts a pipeline from a container using non-const iterators (`std::begin`/`std::end`).
 * @param container Iterable lvalue (must support `begin`/`end`).
 */
template <class C>
[[nodiscard]] auto from(C& container)
{
    return enumerable(from_range(std::begin(container), std::end(container)));
}

/** @brief Const container overload: uses `cbegin`/`cend`. */
template <class C>
[[nodiscard]] auto from(C const& container)
{
    return enumerable(from_range(std::cbegin(container), std::cend(container)));
}

/**
 * @brief Iterator-pair range wrapped in `basic_iterator` adapters.
 * @param b Half-open begin.
 * @param e Half-open end (non-owning).
 */
template <class Iter>
[[nodiscard]] auto from(Iter b, Iter e)
{
    return enumerable(from_range(std::move(b), std::move(e)));
}

/**
 * @brief Alias of `from(b, e)` (explicit “range” wording).
 * @param b Half-open begin.
 * @param e Half-open end.
 */
template <class Iter>
[[nodiscard]] auto range(Iter b, Iter e)
{
    return from(std::move(b), std::move(e));
}

/**
 * @brief Lazy half-open `[first, last)` arithmetic sequence (`iota_view`; no heap).
 * @tparam T Counter type (`++` and equality).
 * @param first `begin()` value.
 * @param last `end()` bound (exclusive iterator position).
 * @see iota_view
 */
template <class T>
[[nodiscard]] auto iota(T first, T last)
{
    return enumerable(iota_view<T>(std::move(first), std::move(last)));
}

/** @brief Ascending key for `order_by` (re-export from `detail`). */
using detail::asc;
/** @brief Descending key for `order_by` (re-export from `detail`). */
using detail::desc;

/**
 * @brief Builds a custom `order_by` key filter with comparator base `CustomBase`.
 * @tparam CustomBase Comparator base (see `detail::order_predicate_base<order_kind::custom>` pattern).
 * @tparam KeyProj Key projection callable.
 * @tparam Args Optional extra ctor args for `CustomBase`.
 * @param key Projection passed to `make_order_filter`.
 * @param args Forwarded to `CustomBase`.
 * @see detail::make_order_filter
 */
template <class CustomBase, class KeyProj, class... Args>
[[nodiscard]] auto make_filter(KeyProj&& key, Args&&... args)
{
    return detail::make_order_filter<CustomBase>(std::forward<KeyProj>(key), std::forward<Args>(args)...);
}

/** @brief CTAD: `enumerable(handle)` deduces `enumerable<std::decay_t<decltype(handle)>>`. */
template <class H>
enumerable(H) -> enumerable<H>;

/**
 * @brief Zero elements, `value_type` is `T` (`empty_view`).
 * @tparam T Element type of the empty sequence.
 */
template <class T>
[[nodiscard]] inline auto empty()
{
    return enumerable(detail::empty_view<T>{});
}

/**
 * @brief Exactly one element (`single_view`).
 * @tparam T Decayed element type.
 * @param value Single element (moved or copied).
 */
template <class T>
[[nodiscard]] inline auto once(T&& value)
{
    return enumerable(detail::single_view<std::decay_t<T>>(std::forward<T>(value)));
}

/**
 * @brief `n` copies of the same value (`repeat_view`).
 * @tparam T Decayed element type.
 * @param value Repeated value.
 * @param n Repeat count.
 */
template <class T>
[[nodiscard]] inline auto repeat(T&& value, std::size_t n)
{
    return enumerable(detail::repeat_view<std::decay_t<T>>(std::forward<T>(value), n));
}

/** @} */

} // namespace qb::linq
