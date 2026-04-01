#pragma once

/**
 * @file query_range.h
 * @ingroup linq
 * @brief CRTP `query_range_algorithms<Derived, Iter>` — lazy transforms, materializers, and eager terminals.
 *
 * @details
 * **CRTP:** `Derived` must expose `begin()` and `end()` returning `Iter`. All algorithms forward to
 * `derived().begin()` / `derived().end()` so `where_view` remains cheap to construct (`find_if` runs only on
 * the first `begin()`, first matching iterator cached in the view — see `where_view` in `query.h`).
 *
 * @par Materialization helpers
 * `reserve_if_random_access(c, b, e)` calls `c.reserve(e - b)` only when `[b,e)` is random-access, avoiding an
 * extra pass for forward iterators. Used by `to_vector`, `order_by`, maps, sets, and join / group_join / outer joins.
 *
 * @par Exceptions
 * Many terminals throw `std::out_of_range` with messages prefixed by `qb::linq::` when preconditions fail
 * (empty sequence for `first`, `single`, `reduce`, etc.). `to_dictionary` throws `std::invalid_argument` on
 * duplicate keys.
 *
 * @par Main symbols (this header)
 * | Name | Role |
 * |------|------|
 * | `reserve_if_random_access` | Optional `reserve` when iterators are random-access |
 * | `of_type_pred` | `dynamic_cast` filter for `of_type<U>` |
 * | `query_range_algorithms` | CRTP mixin: lazy ops, materializers, terminals (see **Method families** on the class) |
 * | Forward decls (`concat_view`, …) | Incomplete friends / return types completed in `extra_views.h` |
 */
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "qb/linq/detail/group_by.h"
#include "qb/linq/detail/order.h"
#include "qb/linq/detail/type_traits.h"

namespace qb::linq {

template <class Iter>
class query_state; ///< Iterator pair handle (`query.h`).

template <class Iter>
class reversed_view; ///< Reverse iteration adapter (`query.h`).

template <class Iter>
class subrange; ///< Half-open subrange (`query.h`).

template <class RawIter>
class from_range; ///< Container-less iterator range (`query.h`).

template <class BaseIt, class F>
class select_view; ///< Lazy projection (`query.h`).

template <class BaseIt, class P>
class where_view; ///< Lazy filter with cached first match (`query.h`).

template <class BaseIt>
class take_n_view; ///< Take first N (`query.h`).

template <class BaseIt, class P>
class take_while_view; ///< Take while predicate (`query.h`).

template <class Iter, class Owner>
class materialized_range; ///< Shared-owned materialized data (`query.h`).

/**
 * @namespace qb::linq::detail
 * @ingroup linq
 * @brief CRTP `query_range_algorithms`, materialization helpers, `of_type_pred`, forward decls for `extra_views.h`.
 */
namespace detail {

/**
 * @brief `b + d` as `Iter` (wraps when `Iter` inherits `BaseIt` and `operator+` returns `BaseIt`, e.g. `basic_iterator`).
 */
template <class Iter>
[[nodiscard]] inline Iter ra_plus(Iter b, typename std::iterator_traits<Iter>::difference_type d)
{
    auto adv = b + d;
    if constexpr (std::is_same_v<std::decay_t<decltype(adv)>, Iter>)
        return adv;
    return Iter(std::move(adv));
}

template <class It1, class It2>
class concat_view; ///< Concatenation of two forward sequences (`extra_views.h`).
template <class It1, class It2>
class zip_view; ///< Pairwise zip; stops at shorter range (`extra_views.h`).
template <class It1, class It2, class It3>
class zip3_view; ///< Three-way zip; stops at shortest range (`extra_views.h`).
template <class It, class T>
class default_if_empty_view; ///< Yields `def` once when source empty (`extra_views.h`).
template <class BaseIt, class KeyFn, class KeyT>
class distinct_view; ///< First occurrence per key (`extra_views.h`).
template <class BaseIt>
class enumerate_view; ///< `(index, element)` pairs (`extra_views.h`).
template <class BaseIt>
class chunk_view; ///< Contiguous slices as `vector` (`extra_views.h`).
template <class BaseIt>
class stride_view; ///< Every k-th element (`extra_views.h`).
template <class BaseIt, class Acc, class F>
class scan_view; ///< Lazy prefix fold / running aggregate (`extra_views.h`).

/**
 * @ingroup linq
 * @brief Calls `c.reserve(e - b)` only for random-access `[b,e)`; no-op otherwise.
 * @param c Container with optional `.reserve`.
 * @param b Range begin.
 * @param e Range end.
 * @see query_range.h file comment (materialization helpers).
 */
template <class Container, class Iter>
void reserve_if_random_access(Container& c, Iter b, Iter e)
{
    using cat = typename std::iterator_traits<Iter>::iterator_category;
    if constexpr (std::is_base_of_v<std::random_access_iterator_tag, cat>) {
        auto const d = e - b;
        if (d > 0)
            c.reserve(static_cast<std::size_t>(d));
    }
}

/**
 * @ingroup linq
 * @brief `dynamic_cast` filter for `of_type<U>`; `U` is the target derived pointee type.
 * @tparam Ref Lvalue reference type of the pointer sequence (`reference` from iterator).
 */
template <class U, class Ref>
struct of_type_pred {
    /** @param p Pointer element; keeps iff `dynamic_cast` to `U*` succeeds. */
    [[nodiscard]] bool operator()(Ref p) const
    {
        return dynamic_cast<std::add_pointer_t<std::remove_cv_t<U>>>(p) != nullptr;
    }
};

/**
 * @ingroup linq
 * @brief CRTP mixin: lazy transforms, relational/set ops, materializers, and scalar terminals.
 * @tparam Derived Most-derived range type (`where_view`, `query_state`, `iota_view`, …).
 * @tparam Iter Iterator type used by `Derived` (explicit to break incomplete-type cycles).
 *
 * @par Method families
 * - **Slicing / transform:** `select`, `select_indexed`, `select_many`, `where`, `where_indexed`, `of_type`,
 *   `skip`, `skip_while`, `take`, `take_while`, `reverse`.
 * - **Composition:** `concat`, `zip` (2- or 3-range), `default_if_empty`, `enumerate`, `scan`, `chunk`, `stride`,
 *   `append`, `prepend`, `distinct`, `distinct_by`, `union_with`.
 * - **Relational:** `join`, `left_join`, `right_join`, `group_join`, `to_lookup` (alias of `group_by(key)`).
 * - **Materialize:** `group_by`, `order_by`, `to_vector`, `to_map`, `to_unordered_map`, `to_dictionary`,
 *   `to_set`, `to_unordered_set`, `except`, `intersect`, `except_by`, `intersect_by`, `union_by`, `count_by`,
 *   `take_last`, `skip_last`.
 * - **Search / quantifiers:** `contains`, `index_of`, `last_index_of`, `sequence_equal`, `any`, `count`,
 *   `try_get_non_enumerated_count`, fused `*_if` helpers, `first`/`last`/`element_at`/`single` families.
 * - **Aggregates / stats:** `sum`, `sum_if`, `average`, `average_if`, `min`/`max`/`min_max`, `min_by`/`max_by`,
 *   `aggregate`/`fold`/`reduce`, percentiles, variance, standard deviation.
 */
template <class Derived, class Iter>
class query_range_algorithms {
    Derived const& derived() const noexcept { return static_cast<Derived const&>(*this); }

public:
    using iterator = Iter;
    using reference = detail::iter_reference_t<Iter>;
    using value_type = std::remove_const_t<std::remove_reference_t<reference>>;

    /** @brief True iff the sequence has at least one element (may advance iterators for single-pass ranges). */
    [[nodiscard]] bool any() const noexcept { return derived().begin() != derived().end(); }

    /** @brief Number of elements (`std::distance` over the range). */
    [[nodiscard]] std::size_t count() const
    {
        return static_cast<std::size_t>(std::distance(derived().begin(), derived().end()));
    }

    /**
     * @brief O(1) length when iterators are random-access; otherwise `nullopt` (no full scan).
     * @details Mirrors the idea of `TryGetNonEnumeratedCount` / sized ranges: forward-only sequences return
     *          `nullopt` even though `count()` could be computed with a linear walk.
     */
    [[nodiscard]] std::optional<std::size_t> try_get_non_enumerated_count() const
    {
        using cat = typename std::iterator_traits<iterator>::iterator_category;
        if constexpr (std::is_base_of_v<std::random_access_iterator_tag, cat>) {
            auto const d = derived().end() - derived().begin();
            if (d < 0)
                return std::nullopt;
            return static_cast<std::size_t>(d);
        }
        return std::nullopt;
    }

    /** @brief First element; throws `std::out_of_range("qb::linq::first")` if empty. @return `reference` into sequence. */
    [[nodiscard]] reference first() const
    {
        iterator it = derived().begin();
        if (it == derived().end())
            throw std::out_of_range("qb::linq::first");
        return *it;
    }

    /** @brief `value_type{}` if empty, else first element (by value). */
    [[nodiscard]] value_type first_or_default() const
    {
        iterator it = derived().begin();
        if (it == derived().end())
            return value_type{};
        return *it;
    }

    /** @brief First element satisfying `pred`; throws `qb::linq::first_if` if none. */
    template <class Pred>
    [[nodiscard]] reference first_if(Pred&& pred) const
    {
        for (iterator it = derived().begin(); it != derived().end(); ++it) {
            if (pred(*it))
                return *it;
        }
        throw std::out_of_range("qb::linq::first_if");
    }

    /** @brief First match or default-constructed `value_type`. */
    template <class Pred>
    [[nodiscard]] value_type first_or_default_if(Pred&& pred) const
    {
        for (iterator it = derived().begin(); it != derived().end(); ++it) {
            if (pred(*it))
                return *it;
        }
        return value_type{};
    }

    /**
     * @brief Last element; throws if empty. Returns a reference into the range (requires bidirectional iterators).
     * @details For forward-only iterators, use `last_or_default()` or `last_if()` instead (these return by value).
     */
    [[nodiscard]] reference last() const
    {
        iterator b = derived().begin();
        iterator const e = derived().end();
        if (b == e)
            throw std::out_of_range("qb::linq::last");
        return *std::prev(e);
    }

    /**
     * @brief Default if empty; else last element by value.
     * @details Works with forward-only iterators: uses `std::prev` for bidirectional+, iterator scan otherwise.
     */
    [[nodiscard]] value_type last_or_default() const
    {
        iterator b = derived().begin();
        iterator const e = derived().end();
        if (b == e)
            return value_type{};
        using cat = typename std::iterator_traits<iterator>::iterator_category;
        if constexpr (std::is_base_of_v<std::bidirectional_iterator_tag, cat>) {
            return *std::prev(e);
        } else {
            iterator prev = b;
            for (++b; b != e; ++b)
                prev = b;
            return *prev;
        }
    }

    /** @brief Last element matching `pred` (full scan); throws if no match. */
    template <class Pred>
    [[nodiscard]] value_type last_if(Pred&& pred) const
    {
        bool found = false;
        value_type out{};
        for (iterator it = derived().begin(); it != derived().end(); ++it) {
            if (pred(*it)) {
                out = *it;
                found = true;
            }
        }
        if (!found)
            throw std::out_of_range("qb::linq::last_if");
        return out;
    }

    /** @brief Last match by value, or default if none. */
    template <class Pred>
    [[nodiscard]] value_type last_or_default_if(Pred&& pred) const
    {
        value_type out{};
        for (iterator it = derived().begin(); it != derived().end(); ++it) {
            if (pred(*it))
                out = *it;
        }
        return out;
    }

    /**
     * @brief Zero-based indexed access (linear from `begin()`).
     * @throws std::out_of_range with message `qb::linq::element_at` if `index` is out of range.
     */
    [[nodiscard]] reference element_at(std::size_t index) const
    {
        iterator b = derived().begin();
        iterator const e = derived().end();
        using cat = typename std::iterator_traits<iterator>::iterator_category;
        if constexpr (std::is_base_of_v<std::random_access_iterator_tag, cat>) {
            auto const dist = e - b;
            if (dist < 0 || static_cast<std::size_t>(dist) <= index)
                throw std::out_of_range("qb::linq::element_at");
            return *detail::ra_plus(b, static_cast<typename std::iterator_traits<iterator>::difference_type>(index));
        } else {
            iterator it = b;
            for (; index > 0 && it != e; --index, ++it) {}
            if (it == e)
                throw std::out_of_range("qb::linq::element_at");
            return *it;
        }
    }

    /** @brief Out-of-range → default-constructed `value_type`. */
    [[nodiscard]] value_type element_at_or_default(std::size_t index) const
    {
        iterator b = derived().begin();
        iterator const e = derived().end();
        using cat = typename std::iterator_traits<iterator>::iterator_category;
        if constexpr (std::is_base_of_v<std::random_access_iterator_tag, cat>) {
            auto const dist = e - b;
            if (dist < 0 || static_cast<std::size_t>(dist) <= index)
                return value_type{};
            return *detail::ra_plus(b, static_cast<typename std::iterator_traits<iterator>::difference_type>(index));
        } else {
            iterator it = b;
            for (; index > 0 && it != e; --index, ++it) {}
            if (it == e)
                return value_type{};
            return *it;
        }
    }

    /** @brief Alias of `count()` (LINQ `LongCount`). */
    [[nodiscard]] std::size_t long_count() const { return count(); }

    /** @brief Same as `count_if(pred)` (LINQ `LongCount` + predicate). */
    template <class Pred>
    [[nodiscard]] std::size_t long_count_if(Pred&& pred) const
    {
        return count_if(std::forward<Pred>(pred));
    }

    /** @brief Pairwise `==` until one range exhausts; `true` iff both end together. */
    template <class Rng>
    [[nodiscard]] bool sequence_equal(Rng const& rhs) const
    {
        iterator a = derived().begin();
        iterator const ae = derived().end();
        auto b = rhs.begin();
        auto const be = rhs.end();
        for (; a != ae && b != be; ++a, ++b) {
            if (!(*a == *b))
                return false;
        }
        return a == ae && b == be;
    }

    /** @brief Element-wise `comp(*a, *b)` until exhaustion. */
    template <class Rng, class Comp>
    [[nodiscard]] bool sequence_equal(Rng const& rhs, Comp comp) const
    {
        iterator a = derived().begin();
        iterator const ae = derived().end();
        auto b = rhs.begin();
        auto const be = rhs.end();
        for (; a != ae && b != be; ++a, ++b) {
            if (!comp(*a, *b))
                return false;
        }
        return a == ae && b == be;
    }

    /** @brief Lazy concatenation; both sides must share the same `value_type`. */
    template <class Rng>
    [[nodiscard]] concat_view<iterator, decltype(std::declval<Rng const&>().begin())> concat(Rng const& rhs) const;

    /** @brief Pairwise zip; ends at the shorter of the two ranges. */
    template <class Rng>
    [[nodiscard]] zip_view<iterator, decltype(std::declval<Rng const&>().begin())> zip(Rng const& rhs) const;

    /**
     * @brief Zip three forward sequences; ends when any leg ends (`std::tuple` of element references per step).
     */
    template <class Rng2, class Rng3>
    [[nodiscard]] zip3_view<iterator, decltype(std::declval<Rng2 const&>().begin()),
        decltype(std::declval<Rng3 const&>().begin())> zip(Rng2 const& r2, Rng3 const& r3) const;

    /**
     * @brief Single-pass fold over zipped pairs (`*this` vs `rhs`); stops at the shorter length.
     * @tparam Acc Accumulator type (decayed from `init`).
     * @tparam F Callable `Acc(Acc&& acc, reference left, RhsRef right)` returning the next accumulator.
     * @details Prefer this over `zip().select(...).aggregate(...)` on hot paths (no `zip_iterator` / pairs).
     */
    template <class Rng, class Acc, class F>
    [[nodiscard]] std::decay_t<Acc> zip_fold(Rng const& rhs, Acc&& init, F&& f) const
    {
        using acc_t = std::decay_t<Acc>;
        iterator a = derived().begin();
        iterator const ae = derived().end();
        auto b = rhs.begin();
        auto const be = rhs.end();
        acc_t acc = static_cast<acc_t>(std::forward<Acc>(init));
        for (; a != ae && b != be; ++a, ++b)
            acc = std::forward<F>(f)(std::move(acc), *a, *b);
        return acc;
    }

    /** @brief When empty, yields `def` once; otherwise forwards the source sequence. */
    [[nodiscard]] default_if_empty_view<iterator, value_type> default_if_empty(value_type def) const;

    [[nodiscard]] default_if_empty_view<iterator, value_type> default_if_empty() const
    {
        return default_if_empty(value_type{});
    }

    /** @brief `(0-based index, element)` for each element (lazy). */
    [[nodiscard]] enumerate_view<iterator> enumerate() const;

    /**
     * @brief Running fold: yields `f(acc, x)` after each element `x`, starting from `seed`.
     * @details Empty source → empty scan. `F` is stored in the returned `scan_view`; iterators hold a non-owning
     *          pointer and must not outlive that view (`extra_views.h`).
     */
    template <class Acc, class F>
    [[nodiscard]] scan_view<iterator, std::decay_t<Acc>, std::decay_t<F>> scan(Acc&& seed, F&& f) const;

    /** @brief Inner join on matching keys; `result(outer, inner)` builds each output row. */
    template <class Rng, class OuterKey, class InnerKey, class ResultSel>
    [[nodiscard]] auto join(
        Rng const& inner, OuterKey&& outer_key, InnerKey&& inner_key, ResultSel&& result) const;

    /** @brief Correlates outer elements with all inner matches (vector of inners per outer key). */
    template <class Rng, class OuterKey, class InnerKey>
    [[nodiscard]] auto group_join(Rng const& inner, OuterKey&& outer_key, InnerKey&& inner_key) const;

    /**
     * @brief Left outer join: every outer row appears at least once; `result(outer, inner_opt)` uses empty
     *        `optional` when no inner key matches (LINQ `GroupJoin` + `DefaultIfEmpty` flat pattern).
     * @note Empty outer → empty result. Empty inner → one row per outer (all with empty `optional` inner).
     */
    template <class Rng, class OuterKey, class InnerKey, class ResultSel>
    [[nodiscard]] auto left_join(
        Rng const& inner, OuterKey&& outer_key, InnerKey&& inner_key, ResultSel&& result) const;

    /**
     * @brief Right outer join: every inner row appears at least once; `result(outer_opt, inner)` uses empty
     *        `optional` when no outer key matches (`right_join` ≡ swapped `left_join`; see README).
     * @note Empty inner → empty result. Empty outer → one row per inner (all with empty `optional` outer).
     */
    template <class Rng, class OuterKey, class InnerKey, class ResultSel>
    [[nodiscard]] auto right_join(
        Rng const& inner, OuterKey&& outer_key, InnerKey&& inner_key, ResultSel&& result) const;

    /** @brief Same as `group_by(key)` (LINQ `ToLookup` naming). */
    template <class KeyFn>
    [[nodiscard]] auto to_lookup(KeyFn&& key) const
    {
        return group_by(std::forward<KeyFn>(key));
    }

    /** @brief Consecutive chunks of up to `size` elements (each chunk is a `std::vector` by value). */
    [[nodiscard]] chunk_view<iterator> chunk(std::size_t size) const;

    /** @brief Every `step`-th element (`step < 1` clamped to 1). */
    [[nodiscard]] stride_view<iterator> stride(std::size_t step) const;

    /** @brief `concat(single_view(v))` — one element after the sequence. */
    template <class T>
    [[nodiscard]] auto append(T&& v) const;

    /** @brief `single_view(v).concat(*this)` — one element before the sequence. */
    template <class T>
    [[nodiscard]] auto prepend(T&& v) const;

    /** @brief Unique elements in unspecified order (`std::unordered_set`). */
    [[nodiscard]] auto to_unordered_set() const;

    /** @brief Unique elements in sorted order (`std::set`). */
    [[nodiscard]] auto to_set() const;

    template <class KF>
    [[nodiscard]] distinct_view<iterator, std::decay_t<KF>,
        std::decay_t<std::invoke_result_t<KF&, reference>>> distinct_by(KF&& kf) const;

    [[nodiscard]] distinct_view<iterator, identity_proj<reference>, value_type> distinct() const;

    /** @brief `concat(rhs).distinct()` — multiset union then unique. */
    template <class Rng>
    [[nodiscard]] auto union_with(Rng const& rhs) const;

    /** @brief Last write wins on duplicate keys (`std::unordered_map`). */
    template <class KeyFn, class ElemFn>
    [[nodiscard]] auto to_unordered_map(KeyFn&& keyf, ElemFn&& elemf) const;

    /** @brief Ordered map (`std::map`); last write wins on duplicate keys. */
    template <class KeyFn, class ElemFn>
    [[nodiscard]] auto to_map(KeyFn&& keyf, ElemFn&& elemf) const;

    /** @brief `unordered_map`; throws if two elements map to the same key. */
    template <class KeyFn, class ElemFn>
    [[nodiscard]] auto to_dictionary(KeyFn&& keyf, ElemFn&& elemf) const;

    /** @brief Set difference vs `rhs` (C# `Enumerable.Except`): **distinct** left values not in `rhs`, first-seen order. */
    template <class Rng>
    [[nodiscard]] materialized_range<typename std::vector<value_type>::iterator, std::vector<value_type>> except(
        Rng const& rhs) const;

    /** @brief Set intersection with `rhs` (C# `Enumerable.Intersect`): **distinct** left values also in `rhs`, first-seen order. */
    template <class Rng>
    [[nodiscard]] materialized_range<typename std::vector<value_type>::iterator, std::vector<value_type>> intersect(
        Rng const& rhs) const;

    /**
     * @brief Set difference by key (C# `Enumerable.ExceptBy`): **distinct** left elements by `key_left(*it)`,
     *        first-seen order, excluding any key present in `rhs` via `key_right(*jt)`.
     * @details Keys must be usable in `std::unordered_set` (hash + equality), as for `except`.
     */
    template <class Rng, class KeyLeft, class KeyRight>
    [[nodiscard]] materialized_range<typename std::vector<value_type>::iterator, std::vector<value_type>> except_by(
        Rng const& rhs, KeyLeft&& key_left, KeyRight&& key_right) const;

    /**
     * @brief Set intersection by key (C# `Enumerable.IntersectBy`): **distinct** left elements by `key_left`,
     *        first-seen order, whose key appears in `rhs` under `key_right`.
     */
    template <class Rng, class KeyLeft, class KeyRight>
    [[nodiscard]] materialized_range<typename std::vector<value_type>::iterator, std::vector<value_type>> intersect_by(
        Rng const& rhs, KeyLeft&& key_left, KeyRight&& key_right) const;

    /**
     * @brief Set union by key (C# `Enumerable.UnionBy`): elements from this sequence, then from `rhs`, each key once;
     *        first-seen order from the left, then first-seen new keys from the right. Same `value_type` on both sides.
     */
    template <class Rng, class KeyLeft, class KeyRight>
    [[nodiscard]] materialized_range<typename std::vector<value_type>::iterator, std::vector<value_type>> union_by(
        Rng const& rhs, KeyLeft&& key_left, KeyRight&& key_right) const;

    /**
     * @brief Counts by key; yields `(key, count)` pairs in **first encounter order** of each key (C# `CountBy`).
     * @details One full pass; keys are stored in `std::unordered_set` / map-style buckets — keys must be hashable.
     */
    template <class KeyFn>
    [[nodiscard]] auto count_by(KeyFn&& keyf) const -> materialized_range<
        typename std::vector<std::pair<std::decay_t<std::invoke_result_t<KeyFn&, reference>>, int>>::iterator,
        std::vector<std::pair<std::decay_t<std::invoke_result_t<KeyFn&, reference>>, int>>>;

    /** @brief Last `n` elements (ring buffer; full scan). */
    [[nodiscard]] materialized_range<typename std::vector<value_type>::iterator, std::vector<value_type>> take_last(
        std::size_t n) const;

    /** @brief Drops last `n` elements after materializing to a vector. */
    [[nodiscard]] materialized_range<typename std::vector<value_type>::iterator, std::vector<value_type>> skip_last(
        std::size_t n) const;

    /** @brief Reverse iteration over the underlying range (requires bidirectional base iterators). */
    [[nodiscard]] reversed_view<iterator> reverse() const { return reversed_view<iterator>(derived()); }

    /** @brief Projects each element with `f` (lazy). */
    template <class F>
    [[nodiscard]] select_view<iterator, std::decay_t<F>> select(F&& f) const
    {
        return select_view<iterator, std::decay_t<F>>(
            derived().begin(), derived().end(), std::forward<F>(f));
    }

    /**
     * @brief `Select` with zero-based index: `f(element, index)` per element (lazy; built on `enumerate`).
     * @details Order matches C# `Select((x, i) => …)` — value first, then index.
     */
    template <class F>
    [[nodiscard]] auto select_indexed(F&& f) const
    {
        return enumerate().select([fn = std::forward<F>(f)](auto const& p) { return fn(p.second, p.first); });
    }

    /**
     * @brief Tuple of one projection per loader per element (not C#-style flattening).
     * @tparam Fs Callables `reference -> ...` (one per column).
     */
    template <class... Fs>
    [[nodiscard]] auto select_many(Fs&&... loaders) const
    {
        auto fn = [loaders...](reference val) {
            return std::tuple<std::invoke_result_t<Fs, reference>...>(loaders(val)...);
        };
        return select_view<iterator, decltype(fn)>(derived().begin(), derived().end(), std::move(fn));
    }

    /** @brief Filters elements with `pred` (lazy; first match may be cached in the view). */
    template <class P>
    [[nodiscard]] where_view<iterator, std::decay_t<P>> where(P&& pred) const
    {
        return where_view<iterator, std::decay_t<P>>(
            derived().begin(), derived().end(), std::forward<P>(pred));
    }

    /**
     * @brief `Where` with index: `pred(element, index)` (lazy; `enumerate` + filter + re-project to element).
     */
    template <class P>
    [[nodiscard]] auto where_indexed(P&& pred) const
    {
        return enumerate()
            .where([pr = std::forward<P>(pred)](auto const& p) { return pr(p.second, p.first); })
            .select([](auto const& p) -> decltype((p.second)) { return p.second; });
    }

    /** @brief Pointer sequence: keeps elements where `dynamic_cast` to `U*` succeeds (LINQ `OfType`). */
    template <class U>
    [[nodiscard]] where_view<iterator, of_type_pred<U, reference>> of_type() const
    {
        static_assert(std::is_pointer_v<value_type>, "qb::linq::of_type requires pointer element type");
        static_assert(std::is_polymorphic_v<std::remove_pointer_t<value_type>>,
            "qb::linq::of_type requires polymorphic pointee (dynamic_cast)");
        return where(of_type_pred<U, reference>{});
    }

    /** @brief Drops the first `offset` elements (linear in `offset`). */
    [[nodiscard]] subrange<iterator> skip(std::size_t offset) const
    {
        auto it = derived().begin();
        for (std::size_t i = 0; i < offset && it != derived().end(); ++it, ++i) {}
        return subrange<iterator>(std::move(it), derived().end());
    }

    /** @brief Skips a leading prefix while `pred` is true. */
    template <class P>
    [[nodiscard]] subrange<iterator> skip_while(P&& pred) const
    {
        auto it = derived().begin();
        while (it != derived().end() && pred(std::cref(*it)))
            ++it;
        return subrange<iterator>(std::move(it), derived().end());
    }

    /** @brief At most `n` elements (`n` may be negative — magnitude used; see `take_n_view`). */
    [[nodiscard]] take_n_view<iterator> take(std::ptrdiff_t n) const
    {
        return take_n_view<iterator>(derived().begin(), derived().end(), n);
    }

    /** @brief Elements from the start while `pred` holds. */
    template <class P>
    [[nodiscard]] take_while_view<iterator, std::decay_t<P>> take_while(P&& pred) const
    {
        return take_while_view<iterator, std::decay_t<P>>(
            derived().begin(), derived().end(), std::forward<P>(pred));
    }

    /** @brief Nested map / vector grouping by one or more key functions (materializing). */
    template <class... Keys>
    [[nodiscard]] materialized_range<typename detail::group_by_map_t<reference, Keys...>::iterator,
        detail::group_by_map_t<reference, Keys...>>
    group_by(Keys&&... keys) const;

    /** @brief Stable sort copy with lexicographic key filters (`asc` / `desc` / `make_filter`). */
    template <class... KeyFilters>
    [[nodiscard]] materialized_range<typename std::vector<value_type>::iterator, std::vector<value_type>>
    order_by(KeyFilters&&... key_filters) const;

    /** @brief Copies elements into `std::vector` (random-access sources may `reserve`). */
    [[nodiscard]] materialized_range<typename std::vector<value_type>::iterator, std::vector<value_type>>
    to_vector() const;

    /** @brief Applies `f` to each element (eager side effect). */
    template <class F>
    void each(F&& f) const
    {
        for (iterator it = derived().begin(); it != derived().end(); ++it)
            f(*it);
    }

    /** @brief Membership test using `operator==` (`std::find`). */
    template <class T>
    [[nodiscard]] bool contains(T const& v) const
    {
        return std::find(derived().begin(), derived().end(), v) != derived().end();
    }

    /** @brief Membership with custom equivalence `eq(*it, v)`. */
    template <class T, class Eq>
    [[nodiscard]] bool contains(T const& v, Eq eq) const
    {
        for (iterator it = derived().begin(); it != derived().end(); ++it) {
            if (eq(*it, v))
                return true;
        }
        return false;
    }

    /** @brief First index where `*it == v`, or `nullopt`. */
    template <class T>
    [[nodiscard]] std::optional<std::size_t> index_of(T const& v) const
    {
        std::size_t i = 0;
        for (iterator it = derived().begin(); it != derived().end(); ++it, ++i) {
            if (*it == v)
                return i;
        }
        return std::nullopt;
    }

    /** @brief First index with `eq(*it, v)`. */
    template <class T, class Eq>
    [[nodiscard]] std::optional<std::size_t> index_of(T const& v, Eq eq) const
    {
        std::size_t i = 0;
        for (iterator it = derived().begin(); it != derived().end(); ++it, ++i) {
            if (eq(*it, v))
                return i;
        }
        return std::nullopt;
    }

    /** @brief Last index with `*it == v`, or `nullopt`. */
    template <class T>
    [[nodiscard]] std::optional<std::size_t> last_index_of(T const& v) const
    {
        std::optional<std::size_t> last;
        std::size_t i = 0;
        for (iterator it = derived().begin(); it != derived().end(); ++it, ++i) {
            if (*it == v)
                last = i;
        }
        return last;
    }

    /** @brief Last index with `eq(*it, v)`. */
    template <class T, class Eq>
    [[nodiscard]] std::optional<std::size_t> last_index_of(T const& v, Eq eq) const
    {
        std::optional<std::size_t> last;
        std::size_t i = 0;
        for (iterator it = derived().begin(); it != derived().end(); ++it, ++i) {
            if (eq(*it, v))
                last = i;
        }
        return last;
    }

    /** @brief Minimum by `operator<`; throws if empty. */
    [[nodiscard]] value_type min() const
    {
        iterator it = derived().begin();
        iterator const e = derived().end();
        if (it == e)
            throw std::out_of_range("qb::linq::min");
        value_type best = *it;
        for (++it; it != e; ++it) {
            if (*it < best)
                best = *it;
        }
        return best;
    }

    /** @brief Maximum by `operator<`; throws if empty. */
    [[nodiscard]] value_type max() const
    {
        iterator it = derived().begin();
        iterator const e = derived().end();
        if (it == e)
            throw std::out_of_range("qb::linq::max");
        value_type best = *it;
        for (++it; it != e; ++it) {
            if (best < *it)
                best = *it;
        }
        return best;
    }

    /** @brief Minimum by strict weak ordering `comp(a, b)`. */
    template <class Comp>
    [[nodiscard]] value_type min(Comp comp) const
    {
        iterator it = derived().begin();
        iterator const e = derived().end();
        if (it == e)
            throw std::out_of_range("qb::linq::min");
        value_type best = *it;
        for (++it; it != e; ++it) {
            if (comp(*it, best))
                best = *it;
        }
        return best;
    }

    /** @brief Maximum by `comp` (same contract as `std::max_element` comparator). */
    template <class Comp>
    [[nodiscard]] value_type max(Comp comp) const
    {
        iterator it = derived().begin();
        iterator const e = derived().end();
        if (it == e)
            throw std::out_of_range("qb::linq::max");
        value_type best = *it;
        for (++it; it != e; ++it) {
            if (comp(best, *it))
                best = *it;
        }
        return best;
    }

    /** @brief One pass: `(min, max)` by `operator<`; throws if empty. */
    [[nodiscard]] std::pair<value_type, value_type> min_max() const
    {
        iterator it = derived().begin();
        iterator const e = derived().end();
        if (it == e)
            throw std::out_of_range("qb::linq::min_max");
        value_type lo = *it;
        value_type hi = *it;
        for (++it; it != e; ++it) {
            if (*it < lo)
                lo = *it;
            if (hi < *it)
                hi = *it;
        }
        return {lo, hi};
    }

    /** @brief `(min, max)` under `comp`. */
    template <class Comp>
    [[nodiscard]] std::pair<value_type, value_type> min_max(Comp comp) const
    {
        iterator it = derived().begin();
        iterator const e = derived().end();
        if (it == e)
            throw std::out_of_range("qb::linq::min_max");
        value_type lo = *it;
        value_type hi = *it;
        for (++it; it != e; ++it) {
            if (comp(*it, lo))
                lo = *it;
            if (comp(hi, *it))
                hi = *it;
        }
        return {lo, hi};
    }

    /** @brief `value_type{}` if empty; else minimum. Single pass (no double iteration). */
    [[nodiscard]] value_type min_or_default() const
    {
        iterator it = derived().begin();
        iterator const e = derived().end();
        if (it == e)
            return value_type{};
        value_type best = *it;
        for (++it; it != e; ++it) {
            if (*it < best)
                best = *it;
        }
        return best;
    }

    /** @brief `value_type{}` if empty; else maximum. Single pass (no double iteration). */
    [[nodiscard]] value_type max_or_default() const
    {
        iterator it = derived().begin();
        iterator const e = derived().end();
        if (it == e)
            return value_type{};
        value_type best = *it;
        for (++it; it != e; ++it) {
            if (best < *it)
                best = *it;
        }
        return best;
    }

    /** @brief Element with minimum `key(*it)` (`key` return type uses `operator<`). */
    template <class KF>
    [[nodiscard]] value_type min_by(KF&& key) const
    {
        iterator it = derived().begin();
        iterator const e = derived().end();
        if (it == e)
            throw std::out_of_range("qb::linq::min_by");
        value_type best = *it;
        auto kb = key(*it);
        for (++it; it != e; ++it) {
            auto const k = key(*it);
            if (k < kb) {
                kb = k;
                best = *it;
            }
        }
        return best;
    }

    /** @brief Element with maximum `key(*it)`. */
    template <class KF>
    [[nodiscard]] value_type max_by(KF&& key) const
    {
        iterator it = derived().begin();
        iterator const e = derived().end();
        if (it == e)
            throw std::out_of_range("qb::linq::max_by");
        value_type best = *it;
        auto kb = key(*it);
        for (++it; it != e; ++it) {
            auto const k = key(*it);
            if (kb < k) {
                kb = k;
                best = *it;
            }
        }
        return best;
    }

    /** @brief `min_by` or default if empty. Single pass. */
    template <class KF>
    [[nodiscard]] value_type min_by_or_default(KF&& key) const
    {
        iterator it = derived().begin();
        iterator const e = derived().end();
        if (it == e)
            return value_type{};
        value_type best = *it;
        auto kb = key(*it);
        for (++it; it != e; ++it) {
            auto const k = key(*it);
            if (k < kb) {
                kb = k;
                best = *it;
            }
        }
        return best;
    }

    /** @brief `max_by` or default if empty. Single pass. */
    template <class KF>
    [[nodiscard]] value_type max_by_or_default(KF&& key) const
    {
        iterator it = derived().begin();
        iterator const e = derived().end();
        if (it == e)
            return value_type{};
        value_type best = *it;
        auto kb = key(*it);
        for (++it; it != e; ++it) {
            auto const k = key(*it);
            if (kb < k) {
                kb = k;
                best = *it;
            }
        }
        return best;
    }

    /** @brief Fused filter + `+=`: no `where_iterator` overhead. */
    template <class Pred>
    [[nodiscard]] value_type sum_if(Pred&& pred) const
    {
        value_type acc{};
        for (iterator it = derived().begin(); it != derived().end(); ++it) {
            if (pred(*it))
                acc += *it;
        }
        return acc;
    }

    /** @brief Number of elements satisfying `pred` (single pass). */
    template <class Pred>
    [[nodiscard]] std::size_t count_if(Pred&& pred) const
    {
        std::size_t n = 0;
        for (iterator it = derived().begin(); it != derived().end(); ++it) {
            if (pred(*it))
                ++n;
        }
        return n;
    }

    /** @brief Short-circuit `true` on first `pred(*it)`. */
    template <class Pred>
    [[nodiscard]] bool any_if(Pred&& pred) const
    {
        for (iterator it = derived().begin(); it != derived().end(); ++it) {
            if (pred(*it))
                return true;
        }
        return false;
    }

    /** @brief Short-circuit `false` on first `!pred(*it)`; `true` for empty. */
    template <class Pred>
    [[nodiscard]] bool all_if(Pred&& pred) const
    {
        for (iterator it = derived().begin(); it != derived().end(); ++it) {
            if (!pred(*it))
                return false;
        }
        return true;
    }

    /** @brief `!any_if(pred)`. */
    template <class Pred>
    [[nodiscard]] bool none_if(Pred&& pred) const
    {
        return !any_if(std::forward<Pred>(pred));
    }

    /** @brief Left fold: `acc = f(std::move(acc), *it)` (LINQ `Aggregate` with seed). */
    template <class Acc, class F>
    [[nodiscard]] Acc aggregate(Acc init, F&& f) const
    {
        Acc acc = std::move(init);
        for (iterator it = derived().begin(); it != derived().end(); ++it)
            acc = f(std::move(acc), *it);
        return acc;
    }

    /** @brief Binary fold without seed; first element is initial `acc` (throws if empty). */
    template <class F>
    [[nodiscard]] value_type reduce(F&& f) const
    {
        iterator b = derived().begin();
        iterator e = derived().end();
        if (b == e)
            throw std::out_of_range("qb::linq::reduce");
        value_type acc = *b;
        for (++b; b != e; ++b)
            acc = f(std::move(acc), *b);
        return acc;
    }

    /** @brief Alias of `aggregate`. */
    template <class Acc, class F>
    [[nodiscard]] Acc fold(Acc init, F&& f) const
    {
        return aggregate(std::move(init), std::forward<F>(f));
    }

    /** @brief Arithmetic mean (`double` accumulation); throws if empty. */
    [[nodiscard]] double average() const
    {
        iterator b = derived().begin();
        iterator e = derived().end();
        if (b == e)
            throw std::out_of_range("qb::linq::average");
        double s = 0;
        std::size_t n = 0;
        for (iterator it = b; it != e; ++it) {
            s += static_cast<double>(*it);
            ++n;
        }
        return s / static_cast<double>(n);
    }

    /** @brief Mean over elements matching `pred`; throws if count is zero. */
    template <class Pred>
    [[nodiscard]] double average_if(Pred&& pred) const
    {
        double s = 0;
        std::size_t n = 0;
        for (iterator it = derived().begin(); it != derived().end(); ++it) {
            if (pred(*it)) {
                s += static_cast<double>(*it);
                ++n;
            }
        }
        if (n == 0)
            throw std::out_of_range("qb::linq::average_if");
        return s / static_cast<double>(n);
    }

    /**
     * @brief Empirical percentile of `key(*it)` after sorting keys (`p` clamped to `[0,100]`).
     * @details Linear interpolation between adjacent order statistics; materializes all keys.
     */
    template <class KeyFn>
    [[nodiscard]] double percentile_by(double p, KeyFn&& key) const
    {
        iterator const b = derived().begin();
        iterator const e = derived().end();
        std::vector<double> keys;
        reserve_if_random_access(keys, b, e);
        for (iterator it = b; it != e; ++it)
            keys.push_back(static_cast<double>(key(*it)));
        if (keys.empty())
            throw std::out_of_range("qb::linq::percentile_by");
        double const pc = std::clamp(p, 0.0, 100.0);
        std::sort(keys.begin(), keys.end());
        std::size_t const n = keys.size();
        if (n == 1)
            return keys[0];
        double const rank = (pc / 100.0) * static_cast<double>(n - 1);
        std::size_t const i = static_cast<std::size_t>(rank);
        double const frac = rank - static_cast<double>(i);
        if (i + 1 >= n)
            return keys[n - 1];
        return keys[i] * (1.0 - frac) + keys[i + 1] * frac;
    }

    /** @brief `percentile_by` with identity key (requires arithmetic `value_type`). */
    [[nodiscard]] double percentile(double p) const
    {
        static_assert(std::is_arithmetic_v<value_type>,
            "qb::linq::percentile requires arithmetic value_type; use percentile_by");
        return percentile_by(p, [](reference x) { return static_cast<double>(x); });
    }

    /** @brief `percentile_by(50, key)`. */
    template <class KeyFn>
    [[nodiscard]] double median_by(KeyFn&& key) const
    {
        return percentile_by(50.0, std::forward<KeyFn>(key));
    }

    /** @brief `percentile(50)` for arithmetic elements. */
    [[nodiscard]] double median() const
    {
        static_assert(std::is_arithmetic_v<value_type>,
            "qb::linq::median requires arithmetic value_type; use median_by");
        return percentile(50.0);
    }

    /** @brief Welford population variance of `key(*it)`; throws if empty. */
    template <class KeyFn>
    [[nodiscard]] double variance_population_by(KeyFn&& key) const
    {
        iterator const b = derived().begin();
        iterator const e = derived().end();
        if (b == e)
            throw std::out_of_range("qb::linq::variance_population_by");
        std::size_t n = 0;
        double mean = 0;
        double m2 = 0;
        for (iterator it = b; it != e; ++it) {
            double const x = static_cast<double>(key(*it));
            ++n;
            double const delta = x - mean;
            mean += delta / static_cast<double>(n);
            double const delta2 = x - mean;
            m2 += delta * delta2;
        }
        return m2 / static_cast<double>(n);
    }

    /** @brief Sample variance (Bessel `n-1`); needs ≥2 elements. */
    template <class KeyFn>
    [[nodiscard]] double variance_sample_by(KeyFn&& key) const
    {
        iterator const b = derived().begin();
        iterator const e = derived().end();
        if (b == e)
            throw std::out_of_range("qb::linq::variance_sample_by");
        std::size_t n = 0;
        double mean = 0;
        double m2 = 0;
        for (iterator it = b; it != e; ++it) {
            double const x = static_cast<double>(key(*it));
            ++n;
            double const delta = x - mean;
            mean += delta / static_cast<double>(n);
            double const delta2 = x - mean;
            m2 += delta * delta2;
        }
        if (n < 2)
            throw std::out_of_range("qb::linq::variance_sample_by: need at least 2 elements");
        return m2 / static_cast<double>(n - 1);
    }

    /** @brief Population variance of elements (arithmetic `value_type` only). */
    [[nodiscard]] double variance_population() const
    {
        static_assert(std::is_arithmetic_v<value_type>,
            "qb::linq::variance_population requires arithmetic value_type; use variance_population_by");
        return variance_population_by([](reference x) { return static_cast<double>(x); });
    }

    /** @brief Sample variance of elements. */
    [[nodiscard]] double variance_sample() const
    {
        static_assert(std::is_arithmetic_v<value_type>,
            "qb::linq::variance_sample requires arithmetic value_type; use variance_sample_by");
        return variance_sample_by([](reference x) { return static_cast<double>(x); });
    }

    /** @brief `sqrt(variance_population_by(key))`. */
    template <class KeyFn>
    [[nodiscard]] double std_dev_population_by(KeyFn&& key) const
    {
        return std::sqrt(variance_population_by(std::forward<KeyFn>(key)));
    }

    /** @brief `sqrt(variance_sample_by(key))`. */
    template <class KeyFn>
    [[nodiscard]] double std_dev_sample_by(KeyFn&& key) const
    {
        return std::sqrt(variance_sample_by(std::forward<KeyFn>(key)));
    }

    /** @brief Population standard deviation of elements. */
    [[nodiscard]] double std_dev_population() const
    {
        return std::sqrt(variance_population());
    }

    /** @brief Sample standard deviation of elements. */
    [[nodiscard]] double std_dev_sample() const
    {
        return std::sqrt(variance_sample());
    }

    /** @brief Exactly one element; throws if length ≠ 1 (LINQ `Single`). */
    [[nodiscard]] reference single() const
    {
        iterator b = derived().begin();
        iterator e = derived().end();
        if (b == e)
            throw std::out_of_range("qb::linq::single: empty sequence");
        if (std::next(b) != e)
            throw std::out_of_range("qb::linq::single: sequence length is not 1");
        return *b;
    }

    /** @brief `value_type{}` unless exactly one element. */
    [[nodiscard]] value_type single_or_default() const
    {
        iterator b = derived().begin();
        iterator e = derived().end();
        if (b == e)
            return value_type{};
        if (std::next(b) != e)
            return value_type{};
        return *b;
    }

    /** @brief Unique match under `pred`; throws if 0 or >1 matches. */
    template <class Pred>
    [[nodiscard]] value_type single_if(Pred&& pred) const
    {
        bool found = false;
        value_type val{};
        for (iterator it = derived().begin(); it != derived().end(); ++it) {
            if (pred(*it)) {
                if (found)
                    throw std::out_of_range("qb::linq::single_if: more than one match");
                val = *it;
                found = true;
            }
        }
        if (!found)
            throw std::out_of_range("qb::linq::single_if: no match");
        return val;
    }

    /** @brief At most one match: value or default; default if ambiguous. */
    template <class Pred>
    [[nodiscard]] value_type single_or_default_if(Pred&& pred) const
    {
        bool found = false;
        value_type val{};
        for (iterator it = derived().begin(); it != derived().end(); ++it) {
            if (pred(*it)) {
                if (found)
                    return value_type{};
                val = *it;
                found = true;
            }
        }
        if (!found)
            return value_type{};
        return val;
    }

    /** @brief `acc += *it` for all elements (`value_type` must support `+=` and default construction). */
    [[nodiscard]] value_type sum() const
    {
        value_type acc{};
        for (iterator it = derived().begin(); it != derived().end(); ++it)
            acc += *it;
        return acc;
    }
};

} // namespace detail

} // namespace qb::linq
