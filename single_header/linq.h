/**
 * @file linq.h
 * @brief Single-header qb-linq (same public API as `#include <qb/linq.h>`).
 *
 * @note Do not edit by hand. Regenerate with:
 * `python scripts/amalgamate_single_header.py` or CMake target **qb_linq_single_header**.
 * @note Embedded version **1.3.0** from `CMakeLists.txt` `project(VERSION ...)`.
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <deque>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <set>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// ---- qb/linq/version.h (embedded) ----

/**
 * @file version.h
 * @ingroup linq
 * @brief Library version — generated from CMake `project(qb-linq VERSION ...)`. Do not edit by hand.
 */

/** @brief Major version (semantic versioning). */
#define QB_LINQ_VERSION_MAJOR 1

/** @brief Minor version. */
#define QB_LINQ_VERSION_MINOR 3

/** @brief Patch version. */
#define QB_LINQ_VERSION_PATCH 0

/** @brief Version string, e.g. `"1.3.0"`. */
#define QB_LINQ_VERSION_STRING "1.3.0"

/**
 * @brief Integer for preprocessor comparison: `major*1'000'000 + minor*1'000 + patch` (minor/patch &lt; 1000).
 * @details Example: 1.2.3 → 1002003. Use `#if QB_LINQ_VERSION_INTEGER >= 1000000` for “at least 1.0.0”.
 */
#define QB_LINQ_VERSION_INTEGER \
    (QB_LINQ_VERSION_MAJOR * 1000000 + QB_LINQ_VERSION_MINOR * 1000 + QB_LINQ_VERSION_PATCH)

namespace qb::linq {

/**
 * @ingroup linq
 * @brief Compile-time library version (mirrors the `QB_LINQ_VERSION_*` macros).
 */
struct version {
    static constexpr int major = QB_LINQ_VERSION_MAJOR;
    static constexpr int minor = QB_LINQ_VERSION_MINOR;
    static constexpr int patch = QB_LINQ_VERSION_PATCH;
    static constexpr int integer = QB_LINQ_VERSION_INTEGER;
};

} // namespace qb::linq

// ---- qb/linq/detail/type_traits.h ----

/**
 * @file type_traits.h
 * @ingroup linq
 * @brief Iterator alias traits and the identity key projection used by `distinct()`.
 *
 * @details
 * Small type utilities shared by `iterators.h`, `query_range.h`, and lazy views. All symbols are in
 * `namespace qb::linq::detail` and are not part of the stable public contract beyond their observable effects
 * on `enumerable` behaviour.
 *
 * @par Symbols
 * | Name | Role |
 * |------|------|
 * | `iter_reference_t` | `std::iterator_traits::reference` |
 * | `iter_value_t` | `std::iterator_traits::value_type` |
 * | `projection_reference_t` | Reference type for `select` projections |
 * | `identity_proj` | Default key = decayed element for `distinct` / `distinct_by` |
 */


namespace qb::linq::detail {

/**
 * @ingroup linq
 * @brief Reference type obtained by dereferencing `Iterator` (`std::iterator_traits`).
 * @tparam Iterator Iterator type.
 */
template <class Iterator>
using iter_reference_t = typename std::iterator_traits<Iterator>::reference;

/**
 * @ingroup linq
 * @brief Value type of `Iterator` (`std::iterator_traits::value_type`).
 * @tparam Iterator Iterator type.
 */
template <class Iterator>
using iter_value_t = typename std::iterator_traits<Iterator>::value_type;

/**
 * @ingroup linq
 * @brief Reference type for `select` projections: preserves references from `R`, otherwise uses the value type.
 * @tparam R Type returned by the projection callable (may be `T&`, `T&&`, or `T`).
 */
template <class R>
using projection_reference_t =
    std::conditional_t<std::is_reference_v<R>, R, std::remove_reference_t<R>>;

/**
 * @ingroup linq
 * @brief Default key function for `distinct()`: returns a decayed copy of the element for `unordered_set`.
 * @tparam Ref Reference type of the source sequence (typically `iterator::reference`).
 */
template <class Ref>
struct identity_proj {
    /** @name Call operator */
    /** @{ */
    /**
     * @brief Key for `distinct_by` / `distinct`: decayed element value (hashable / comparable as `KeyT` requires).
     * @tparam T Typically `reference` from the sequence; universal reference.
     */
    template <class T>
    [[nodiscard]] constexpr std::decay_t<T> operator()(T&& x) const
    {
        return std::forward<T>(x);
    }
    /** @} */
};

} // namespace qb::linq::detail

// ---- qb/linq/detail/order.h ----

/**
 * @file order.h
 * @ingroup linq
 * @brief Building blocks for `order_by`: ascending / descending / custom comparators and key wrappers.
 *
 * @details
 * `asc(key)` and `desc(key)` produce `order_key_filter` objects consumed by `order_by`. Multi-key sorts use
 * lexicographic comparison: `lexicographic_compare(a, b, f1, f2, ...)` applies `primary` / `tied` on each
 * filter in order. Custom comparators extend `order_predicate_base<order_kind::custom>` (see `make_filter`
 * in `enumerable.h`).
 *
 * @par Symbols
 * | Name | Role |
 * |------|------|
 * | `order_kind` | `ascending` / `descending` / `custom` discriminator |
 * | `order_predicate_base` | `operator()` + `next` for strict order and tie-break |
 * | `order_key_filter` | Key projection + base comparator |
 * | `lexicographic_compare` | Multi-key less-than |
 * | `ascending_key_t` / `descending_key_t` | Result aliases of `asc` / `desc` |
 * | `make_order_filter` | Build custom `order_key_filter` |
 * | `asc` / `desc` | Factory functions for `order_by` keys |
 */


namespace qb::linq::detail {

/**
 * @ingroup linq
 * @brief Selects which `order_predicate_base` specialization backs an `order_key_filter`.
 * @see order_predicate_base
 */
enum class order_kind { ascending, descending, custom };

/**
 * @ingroup linq
 * @brief Primary template; use specializations for `ascending`, `descending`, or `custom` ordering.
 */
template <order_kind Kind, class = void>
struct order_predicate_base;

/**
 * @ingroup linq
 * @brief Ascending comparison: `primary` uses `<`, tie-break uses `==`.
 */
template <>
struct order_predicate_base<order_kind::ascending> {
    template <class L, class R>
    constexpr bool operator()(L const& lhs, R const& rhs) const noexcept(noexcept(lhs < rhs))
    {
        return lhs < rhs;
    }

    template <class L, class R>
    constexpr bool next(L const& lhs, R const& rhs) const noexcept(noexcept(lhs == rhs))
    {
        return lhs == rhs;
    }
};

/**
 * @ingroup linq
 * @brief Descending comparison: `primary` uses `>`, tie-break uses `==`.
 */
template <>
struct order_predicate_base<order_kind::descending> {
    template <class L, class R>
    constexpr bool operator()(L const& lhs, R const& rhs) const noexcept(noexcept(lhs > rhs))
    {
        return lhs > rhs;
    }

    template <class L, class R>
    constexpr bool next(L const& lhs, R const& rhs) const noexcept(noexcept(lhs == rhs))
    {
        return lhs == rhs;
    }
};

/**
 * @ingroup linq
 * @brief Placeholder base for custom `order_predicate` types (overridden by `make_order_filter`).
 */
template <>
struct order_predicate_base<order_kind::custom> {
    template <class L, class R>
    constexpr bool operator()(L const&, R const&) const noexcept
    {
        return false;
    }

    template <class L, class R>
    constexpr bool next(L const&, R const&) const noexcept
    {
        return false;
    }
};

/**
 * @ingroup linq
 * @brief Wraps a key projection and base comparator for use in `lexicographic_compare`.
 * @tparam BasePredicate Typically `order_predicate_base<ascending|descending>` or a user type.
 * @tparam KeyProj Callable stored by value (safe for temporaries such as lambdas).
 * @tparam BaseCtorArgs Optional constructor arguments forwarded to `BasePredicate`.
 */
template <class BasePredicate, class KeyProj, class... BaseCtorArgs>
class order_key_filter : public BasePredicate {
public:
    /** @name Construction */
    /** @{ */
    /**
     * @brief Stores `key` and forwards `args` to `BasePredicate`.
     * @param key Key projection.
     * @param args Optional arguments for `BasePredicate`.
     */
    template <class K, class... Args>
    order_key_filter(K&& key, Args&&... args)
        : BasePredicate(std::forward<Args>(args)...), key_(std::forward<K>(key))
    {}
    /** @} */

    /** @name Lexicographic level */
    /** @{ */
    /** @brief Strict ordering on projected keys (`BasePredicate` applied to `key(lhs)`, `key(rhs)`). */
    template <class L, class R>
    constexpr bool primary(L const& lhs, R const& rhs) const
        noexcept(noexcept(std::declval<BasePredicate const&>()(key_(lhs), key_(rhs))))
    {
        return static_cast<BasePredicate const&>(*this)(key_(lhs), key_(rhs));
    }

    /** @brief True when keys are tied on this level (delegates to `next` on projected keys). */
    template <class L, class R>
    constexpr bool tied(L const& lhs, R const& rhs) const
        noexcept(noexcept(std::declval<BasePredicate const&>().next(key_(lhs), key_(rhs))))
    {
        return static_cast<BasePredicate const&>(*this).next(key_(lhs), key_(rhs));
    }
    /** @} */

private:
    KeyProj key_;
};

/**
 * @ingroup linq
 * @brief Lexicographic less-than on multi-key `order_by` filters.
 * @tparam In Element type (reference to sorted values).
 * @tparam Filter First key filter (`primary` / `tied`).
 * @tparam Filters Remaining key filters.
 * @return True if `a` is ordered before `b` under `(head, tail...)`.
 */
template <class In, class Filter, class... Filters>
constexpr bool lexicographic_compare(In const& a, In const& b, Filter const& head, Filters&&... tail)
{
    if (head.primary(a, b))
        return true;
    if (head.tied(a, b))
        return lexicographic_compare(a, b, std::forward<Filters>(tail)...);
    return false;
}

/**
 * @ingroup linq
 * @brief Single-key ordering: `head.primary(a,b)`.
 */
template <class In, class Filter>
constexpr bool lexicographic_compare(In const& a, In const& b, Filter const& head)
{
    return head.primary(a, b);
}

/**
 * @ingroup linq
 * @brief `asc(key)` result type: `order_key_filter` + ascending `<` / `==` on projected keys.
 */
template <class KeyProj>
using ascending_key_t = order_key_filter<order_predicate_base<order_kind::ascending>,
                                         std::decay_t<KeyProj>>;

/**
 * @ingroup linq
 * @brief `desc(key)` result type: `order_key_filter` + descending `>` / `==` on projected keys.
 */
template <class KeyProj>
using descending_key_t = order_key_filter<order_predicate_base<order_kind::descending>,
                                          std::decay_t<KeyProj>>;

/**
 * @ingroup linq
 * @brief Builds an `order_key_filter` with a custom comparator base.
 * @tparam CustomBase Comparator type (e.g. your `struct` with `operator()` and `next`).
 * @param key Key projection (stored by decayed value in the filter).
 * @param args Forwarded to `CustomBase` constructor after the key.
 */
template <class CustomBase, class KeyProj, class... Args>
order_key_filter<CustomBase, std::decay_t<KeyProj>, Args...> make_order_filter(
    KeyProj&& key, Args&&... args)
{
    return order_key_filter<CustomBase, std::decay_t<KeyProj>, Args...>(
        std::forward<KeyProj>(key), std::forward<Args>(args)...);
}

/**
 * @ingroup linq
 * @brief Ascending sort key for `order_by` (LINQ `OrderBy`).
 * @param key Callable `element -> sort_key` for primary ascending comparison.
 */
template <class KeyProj>
ascending_key_t<KeyProj> asc(KeyProj&& key)
{
    return ascending_key_t<KeyProj>(std::forward<KeyProj>(key));
}

/**
 * @ingroup linq
 * @brief Descending sort key for `order_by` (LINQ `OrderByDescending`).
 * @param key Callable `element -> sort_key` for primary descending comparison.
 */
template <class KeyProj>
descending_key_t<KeyProj> desc(KeyProj&& key)
{
    return descending_key_t<KeyProj>(std::forward<KeyProj>(key));
}

} // namespace qb::linq::detail

// ---- qb/linq/detail/group_by.h ----

/**
 * @file group_by.h
 * @ingroup linq
 * @brief Compile-time nested map / vector layout and insertion helpers for `group_by`.
 *
 * @details
 * `group_by_map_t<reference, KeyFns...>` is the owner type materialized by `query_range_algorithms::group_by`.
 * Fundamental key types use `std::unordered_map`; otherwise `std::map` (ordered) is selected so composite
 * keys remain well-formed. The leaf level is always `std::vector` of value-stored elements.
 *
 * @par Symbols
 * | Name | Role |
 * |------|------|
 * | `map_type_impl` / `map_type_for_key_t` | Pick `unordered_map` vs `map` per key type |
 * | `group_by_impl` | Recursive nested type + `emplace` into the tree |
 * | `group_by_map_t` | Alias to the concrete nested container type |
 */


namespace qb::linq::detail {

/**
 * @ingroup linq
 * @brief Selects `unordered_map` vs `map` for a grouping level based on key type.
 * @tparam Key Key type produced by a key function.
 * @tparam Value Nested map type or `std::vector` of elements at the leaf.
 * @tparam use_unordered If true, use `std::unordered_map<Key, Value>`.
 */
template <class Key, class Value, bool use_unordered>
struct map_type_impl {
    using type = std::unordered_map<Key, Value>;
};

/**
 * @ingroup linq
 * @brief Non-fundamental keys: ordered `std::map` (composite / non-hash-friendly keys).
 */
template <class Key, class Value>
struct map_type_impl<Key, Value, false> {
    using type = std::map<Key, Value>;
};

/**
 * @ingroup linq
 * @brief Map type for one `group_by` level: unordered if `Key` is fundamental, else ordered map.
 */
template <class Key, class Value>
using map_type_for_key_t = typename map_type_impl<Key, Value,
                                                std::is_fundamental_v<Key>>::type;

/**
 * @ingroup linq
 * @brief Recursive description of the nested structure built by `group_by(keys...)`.
 * @tparam In Element type (reference or value) passed to key functions.
 * @tparam KeyFns Key projection callables, outermost first.
 */
template <class In, class... KeyFns>
struct group_by_impl;

/**
 * @ingroup linq
 * @brief Non-terminal level: map from `K0(val)` into nested `group_by_impl<In, Rest...>`.
 */
template <class In, class K0, class... Rest>
struct group_by_impl<In, K0, Rest...> {
    using key_result_t = std::invoke_result_t<K0, In>;
    using nested_t = typename group_by_impl<In, Rest...>::type;
    using type = map_type_for_key_t<key_result_t, nested_t>;

    /**
     * @brief Inserts `val` along the key path `k0(rest...)(val)`.
     */
    template <class... Fs>
    static void emplace(type& handle, In const& val, K0 const& k0, Fs const&... rest)
    {
        group_by_impl<In, Rest...>::emplace(handle[k0(val)], val, rest...);
    }
};

/**
 * @ingroup linq
 * @brief One key left before the leaf: map from `K0(val)` into the leaf vector type.
 */
template <class In, class K0>
struct group_by_impl<In, K0> {
    using key_result_t = std::invoke_result_t<K0, In>;
    using leaf_t = typename group_by_impl<In>::type;
    using type = map_type_for_key_t<key_result_t, leaf_t>;

    static void emplace(type& handle, In const& val, K0 const& k0)
    {
        group_by_impl<In>::emplace(handle[k0(val)], val);
    }
};

/**
 * @ingroup linq
 * @brief Leaf: vector of elements stored by value (`std::vector<const T>` is ill-formed).
 */
template <class In>
struct group_by_impl<In> {
    using element_t = std::remove_cv_t<std::remove_reference_t<In>>;
    using type = std::vector<element_t>;

    /**
     * @brief Appends `val` to the leaf `std::vector`.
     * @param vec Leaf bucket for one terminal key path.
     * @param val Stored as `remove_cvref_t<In>`.
     */
    static void emplace(type& vec, In const& val) { vec.emplace_back(static_cast<element_t>(val)); }
};

/**
 * @ingroup linq
 * @brief Concrete nested map / vector type for `group_by` with the given key functions.
 * @tparam In Reference (or value) type of source elements.
 * @tparam Funcs Key function types.
 */
template <class In, class... Funcs>
using group_by_map_t = typename group_by_impl<In, Funcs...>::type;

} // namespace qb::linq::detail

// ---- qb/linq/detail/lazy_copy_once.h ----

/**
 * @file lazy_copy_once.h
 * @ingroup linq
 * @brief Holds a `T` at most once, lazily initialized, without requiring `T` to be default-constructible.
 *
 * @details
 * Used by `where_view` to cache `std::find_if`’s result. `std::optional<T>` is ill-suited when `T` is not
 * default-constructible when paired with some standard reverse adaptors. This type keeps an
 * **empty** state with **no** `T` object until `cref_or_emplace` runs, then copies/moves `T` on copy/move of
 * the holder. **No heap allocation.**
 */


namespace qb::linq::detail {

/**
 * @ingroup linq
 * @brief Lazy-initialized value; supports copy/move; `const` initialization from a nullary invocable.
 */
template <class T>
class lazy_copy_once {
    static_assert(!std::is_reference_v<T>, "lazy_copy_once<T>: T must not be a reference");
    static_assert(!std::is_const_v<T>, "lazy_copy_once<T>: T must not be const-qualified");

    alignas(T) mutable unsigned char buf_[sizeof(T)]{};
    mutable bool engaged_{false};

    T* ptr() noexcept { return std::launder(reinterpret_cast<T*>(buf_)); }
    T const* ptr() const noexcept { return std::launder(reinterpret_cast<T const*>(buf_)); }

    void destroy() noexcept
    {
        if (engaged_) {
            ptr()->~T();
            engaged_ = false;
        }
    }

public:
    lazy_copy_once() noexcept = default;

    lazy_copy_once(lazy_copy_once const& other) noexcept(std::is_nothrow_copy_constructible_v<T>)
        : engaged_(false)
    {
        if (other.engaged_) {
            new (buf_) T(*other.ptr());
            engaged_ = true;
        }
    }

    lazy_copy_once(lazy_copy_once&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        : engaged_(false)
    {
        if (other.engaged_) {
            new (buf_) T(std::move(*other.ptr()));
            engaged_ = true;
            other.destroy();
        }
    }

    lazy_copy_once& operator=(lazy_copy_once const& other) noexcept(
        std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_destructible_v<T>)
    {
        if (this == &other)
            return *this;
        destroy();
        if (other.engaged_) {
            new (buf_) T(*other.ptr());
            engaged_ = true;
        }
        return *this;
    }

    lazy_copy_once& operator=(lazy_copy_once&& other) noexcept(
        std::is_nothrow_move_constructible_v<T> && std::is_nothrow_destructible_v<T>)
    {
        if (this == &other)
            return *this;
        destroy();
        if (other.engaged_) {
            new (buf_) T(std::move(*other.ptr()));
            engaged_ = true;
            other.destroy();
        }
        return *this;
    }

    ~lazy_copy_once() { destroy(); }

    /**
     * @brief If empty, constructs `T` from `f()`; returns const reference to the stored `T`.
     * @param f Nullary callable returning `T` (typically a lambda that runs `find_if`).
     */
    template <class F>
    [[nodiscard]] T const& cref_or_emplace(F&& f) const
    {
        if (!engaged_) {
            new (buf_) T(std::forward<F>(f)());
            engaged_ = true;
        }
        return *ptr();
    }
};

} // namespace qb::linq::detail

// ---- qb/linq/detail/query_range.h ----

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

// ---- qb/linq/iterators.h ----

/**
 * @file iterators.h
 * @ingroup linq
 * @brief Iterator adaptors backing lazy `select`, `where`, `take`, and `take_while` views.
 *
 * @details
 * These types are implementation details of `query.h` views but live in `namespace qb::linq` because they
 * compose with `query_state` and `query_range_algorithms`. Category is **downgraded** from random-access to
 * bidirectional for `where_iterator`, `take_while_iterator`, and `take_n_iterator` so `std::distance` never
 * assumes pointer-like jumps over filtered elements.
 *
 * @par Types in this header
 * | Type | Role |
 * |------|------|
 * | `detail::clamp_iterator_category_t` | SFINAE alias: cap random-access at bidirectional for adaptors |
 * | `basic_iterator` | Class-type wrapper over raw iterator (`from_range`) |
 * | `select_iterator` | Lazy projection (`Select`) |
 * | `where_iterator` | Skip-until-predicate (`Where`) |
 * | `take_while_iterator` | Prefix while predicate (`TakeWhile`) |
 * | `take_n_iterator` | Prefix of length N (`Take`) |
 * | `detail::compressed_fn<F>` | Empty-base storage for `F` in `select` / `where` / `take_while` iterators when `F` is an empty class |
 *
 * @par MSVC note
 * Iterator adaptors and `compressed_fn<F,true>` use `__declspec(empty_bases)` so an empty `F` participates in
 * EBO under multiple inheritance (otherwise `sizeof` can be `sizeof(BaseIt)` plus a full padding slot).
 */



/** MSVC: enable empty-base optimization across multiple inheritance (otherwise `sizeof` can double). */
#if defined(_MSC_VER)
#define QB_LINQ_ITER_EMPTY_BASES __declspec(empty_bases)
#else
#define QB_LINQ_ITER_EMPTY_BASES
#endif

namespace qb::linq {

namespace detail {

/**
 * @ingroup linq
 * @brief Downgrades random-access to bidirectional for filtered / counted adaptors.
 * @details Prevents O(1) `distance` via pointer subtraction when elements are skipped logically.
 */
template <class Cat>
using clamp_iterator_category_t =
    std::conditional_t<std::is_base_of_v<std::random_access_iterator_tag, Cat>,
        std::bidirectional_iterator_tag, Cat>;

/**
 * @brief Stores `F` with empty-base optimization when `F` is an empty class type (C++17).
 */
template <class F, bool Ebo = std::is_class_v<F>&& std::is_empty_v<F>&& !std::is_final_v<F>>
struct compressed_fn;

template <class F>
struct QB_LINQ_ITER_EMPTY_BASES compressed_fn<F, true> : private F {
    explicit compressed_fn(F fn) noexcept(std::is_nothrow_move_constructible_v<F>)
        : F(std::move(fn))
    {}
    F& fn() noexcept { return static_cast<F&>(*this); }
    F const& fn() const noexcept { return static_cast<F const&>(*this); }
};

template <class F>
struct compressed_fn<F, false> {
    F fn_;

    explicit compressed_fn(F fn) noexcept(std::is_nothrow_move_constructible_v<F>)
        : fn_(std::move(fn))
    {}
    F& fn() noexcept { return fn_; }
    F const& fn() const noexcept { return fn_; }
};

} // namespace detail

/**
 * @ingroup linq
 * @brief Thin wrapper that promotes a raw iterator to a class type with consistent `operator++`/`--`.
 * @tparam BaseIt Underlying iterator (used by `from_range`).
 */
template <class BaseIt>
class basic_iterator : public BaseIt {
public:
    using iterator_category = typename std::iterator_traits<BaseIt>::iterator_category;
    using value_type = typename std::iterator_traits<BaseIt>::value_type;
    using difference_type = typename std::iterator_traits<BaseIt>::difference_type;
    using pointer = typename std::iterator_traits<BaseIt>::pointer;
    using reference = typename std::iterator_traits<BaseIt>::reference;

    /** @name Construction (basic_iterator) */
    /** @{ */
    basic_iterator() = delete;
    basic_iterator(basic_iterator const&) = default;
    basic_iterator& operator=(basic_iterator const&) = default;

    /**
     * @brief Wraps `base` so the iterator is a class type with uniform `++`/`--`.
     * @param base Underlying iterator value (moved into the adaptor).
     */
    explicit basic_iterator(BaseIt base) noexcept(std::is_nothrow_move_constructible_v<BaseIt>)
        : BaseIt(std::move(base))
    {}
    /** @} */

    /** @name Traversal */
    /** @{ */
    /** @brief Pre-increment underlying iterator. */
    basic_iterator& operator++() noexcept(noexcept(++std::declval<BaseIt&>()))
    {
        ++static_cast<BaseIt&>(*this);
        return *this;
    }

    /** @brief Post-increment. */
    basic_iterator operator++(int)
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    /** @brief Pre-decrement (requires bidirectional `BaseIt`). */
    basic_iterator& operator--() noexcept(noexcept(--std::declval<BaseIt&>()))
    {
        --static_cast<BaseIt&>(*this);
        return *this;
    }

    /** @brief Post-decrement. */
    basic_iterator operator--(int)
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }
    /** @} */
};

/**
 * @ingroup linq
 * @brief Projects each dereferenced element through `F` (lazy `Select`).
 * @tparam BaseIt Source iterator.
 * @tparam F Callable `reference -> R` (reference type may preserve `R` as lvalue/rvalue ref).
 */
template <class BaseIt, class F>
class QB_LINQ_ITER_EMPTY_BASES select_iterator : public BaseIt, private detail::compressed_fn<F> {
    using fn_holder = detail::compressed_fn<F>;

public:
    using iterator_category = typename std::iterator_traits<BaseIt>::iterator_category;
    using projected_t =
        std::invoke_result_t<F const&, decltype(*std::declval<BaseIt const&>())>;
    using value_type = std::remove_reference_t<projected_t>;
    using difference_type = typename std::iterator_traits<BaseIt>::difference_type;
    using pointer = value_type*;
    using reference = detail::projection_reference_t<projected_t>;

    /** @name Construction (select_iterator) */
    /** @{ */
    select_iterator() = delete;
    select_iterator(select_iterator const&) = default;

    /**
     * @brief Holds `base` and `fn`; each `operator*` applies `fn(*base)`.
     * @param base Current position in the source range.
     * @param fn Projection; invoked on each `operator*` (stored in the iterator).
     */
    select_iterator(BaseIt base, F fn) noexcept(
        std::is_nothrow_move_constructible_v<BaseIt>&& std::is_nothrow_move_constructible_v<F>)
        : BaseIt(std::move(base)), fn_holder(std::move(fn))
    {}

    /**
     * @brief Copy-assign underlying position from `rhs` (does not assign `fn_`; MSVC `find_if` assigns iterators,
     *        and lambdas with reference captures are not copy-assignable — valid only within the same pipeline).
     */
    select_iterator& operator=(select_iterator const& rhs) noexcept(
        noexcept(std::declval<BaseIt&>() = static_cast<BaseIt const&>(rhs)))
    {
        static_cast<BaseIt&>(*this) = static_cast<BaseIt const&>(rhs);
        return *this;
    }
    /** @} */

    /** @name Traversal */
    /** @{ */
    /** @brief Advance underlying iterator. */
    select_iterator& operator++() noexcept(noexcept(++std::declval<BaseIt&>()))
    {
        ++static_cast<BaseIt&>(*this);
        return *this;
    }

    /** @brief Post-increment. */
    select_iterator operator++(int)
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    /** @brief Retreat underlying iterator. */
    select_iterator& operator--() noexcept(noexcept(--std::declval<BaseIt&>()))
    {
        --static_cast<BaseIt&>(*this);
        return *this;
    }

    /** @brief Post-decrement. */
    select_iterator operator--(int)
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }
    /** @} */

    /** @name Dereference */
    /** @{ */
    /** @brief Projected value from `fn(*base)` (const path). */
    [[nodiscard]] reference operator*() const
        noexcept(noexcept(std::declval<F const&>()(*std::declval<BaseIt const&>())))
    {
        return static_cast<fn_holder const&>(*this).fn()(*static_cast<BaseIt const&>(*this));
    }

    /** @brief Projected value from `fn(*base)` (mutable path). */
    [[nodiscard]] reference operator*() noexcept(noexcept(std::declval<F const&>()(*std::declval<BaseIt&>())))
    {
        return static_cast<fn_holder&>(*this).fn()(*static_cast<BaseIt&>(*this));
    }
    /** @} */
};

/**
 * @ingroup linq
 * @brief Skips elements that fail `Pred`; supports forward and reverse traversal (LINQ `Where`).
 * @tparam BaseIt Source iterator.
 * @tparam Pred Predicate `reference -> bool`.
 * @note Iterator category is at most bidirectional even if `BaseIt` is random-access.
 */
template <class BaseIt, class Pred>
class QB_LINQ_ITER_EMPTY_BASES where_iterator : public BaseIt, private detail::compressed_fn<Pred> {
    using pred_holder = detail::compressed_fn<Pred>;

public:
    using iterator_category = detail::clamp_iterator_category_t<
        typename std::iterator_traits<BaseIt>::iterator_category>;
    using value_type = typename std::iterator_traits<BaseIt>::value_type;
    using difference_type = typename std::iterator_traits<BaseIt>::difference_type;
    using pointer = typename std::iterator_traits<BaseIt>::pointer;
    using reference = typename std::iterator_traits<BaseIt>::reference;

    /** @name Construction (where_iterator) */
    /** @{ */
    where_iterator() = delete;
    where_iterator(where_iterator const&) = default;

    /**
     * @brief Iterator at `current`, with physical `[begin,end)` and `pred` for forward/backward skip.
     * @param current Start position (typically first match or `begin` after `find_if`).
     * @param begin Physical start of the underlying range (reverse / `++` boundary).
     * @param end Physical end; also used as “no match” sentinel in reverse.
     * @param pred Element predicate.
     */
    where_iterator(BaseIt current, BaseIt begin, BaseIt end, Pred pred)
        : BaseIt(std::move(current))
        , pred_holder(std::move(pred))
        , begin_(std::move(begin))
        , end_(std::move(end))
    {}

    /**
     * @brief Copy-assign positions and range bounds from `rhs` (does not assign `pred_`; see `select_iterator::operator=`).
     */
    where_iterator& operator=(where_iterator const& rhs) noexcept(
        noexcept(std::declval<BaseIt&>() = std::declval<BaseIt const&>()))
    {
        static_cast<BaseIt&>(*this) = static_cast<BaseIt const&>(rhs);
        begin_ = rhs.begin_;
        end_ = rhs.end_;
        return *this;
    }
    /** @} */

    /** @name Traversal */
    /** @{ */
    /** @brief Advance to next element satisfying `pred`; stop at `end_`. */
    where_iterator& operator++()
    {
        Pred const& pr = static_cast<pred_holder const&>(*this).fn();
        do {
            ++static_cast<BaseIt&>(*this);
        } while (static_cast<BaseIt const&>(*this) != end_ && !pr(*static_cast<BaseIt const&>(*this)));
        return *this;
    }

    /** @brief Post-increment. */
    where_iterator operator++(int)
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    /** @brief Step backward to previous match, or to `end_` sentinel when leaving the first match. */
    where_iterator& operator--()
    {
        BaseIt& cur = static_cast<BaseIt&>(*this);
        Pred const& pr = static_cast<pred_holder const&>(*this).fn();

        // Logical end sits at underlying \p end_; do not dereference there.
        if (cur == end_) {
            if (begin_ == end_)
                return *this;
            --cur;
            while (cur != begin_ && !pr(*cur))
                --cur;
            if (!pr(*cur))
                cur = end_;
            return *this;
        }

        // Past-the-first-match in reverse order is represented as \p end_.
        if (cur == begin_) {
            cur = end_;
            return *this;
        }

        --cur;
        while (cur != begin_ && !pr(*cur))
            --cur;
        if (!pr(*cur))
            cur = end_;
        return *this;
    }

    /** @brief Post-decrement. */
    where_iterator operator--(int)
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }
    /** @} */

private:
    BaseIt begin_;
    BaseIt end_;
};

/**
 * @ingroup linq
 * @brief Logical range ending when `Pred` becomes false or the physical end is reached (`TakeWhile`).
 * @tparam BaseIt Source iterator.
 * @tparam Pred Predicate; must not be invoked on the end iterator of the peer range in `operator==`.
 */
template <class BaseIt, class Pred>
class QB_LINQ_ITER_EMPTY_BASES take_while_iterator : public BaseIt, private detail::compressed_fn<Pred> {
    using pred_holder = detail::compressed_fn<Pred>;

public:
    using iterator_category = detail::clamp_iterator_category_t<
        typename std::iterator_traits<BaseIt>::iterator_category>;
    using value_type = typename std::iterator_traits<BaseIt>::value_type;
    using difference_type = typename std::iterator_traits<BaseIt>::difference_type;
    using pointer = typename std::iterator_traits<BaseIt>::pointer;
    using reference = typename std::iterator_traits<BaseIt>::reference;

    /** @name Construction (take_while_iterator) */
    /** @{ */
    take_while_iterator() = delete;
    take_while_iterator(take_while_iterator const&) = default;

    /**
     * @brief Pair `base` with `pred`; equality defines logical end (see `operator==`).
     */
    take_while_iterator(BaseIt base, Pred pred) noexcept(
        std::is_nothrow_move_constructible_v<BaseIt>&& std::is_nothrow_move_constructible_v<Pred>)
        : BaseIt(std::move(base)), pred_holder(std::move(pred))
    {}
    /** @} */

    /** @name Traversal */
    /** @{ */
    /** @brief Advance underlying iterator (caller pairs with `operator==` for logical end). */
    take_while_iterator& operator++() noexcept(noexcept(++std::declval<BaseIt&>()))
    {
        ++static_cast<BaseIt&>(*this);
        return *this;
    }

    /** @brief Post-increment. */
    take_while_iterator operator++(int)
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    /** @brief Retreat underlying iterator. */
    take_while_iterator& operator--() noexcept(noexcept(--std::declval<BaseIt&>()))
    {
        --static_cast<BaseIt&>(*this);
        return *this;
    }

    /** @brief Post-decrement. */
    take_while_iterator operator--(int)
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }
    /** @} */

    /** @name Comparison */
    /** @{ */
    /**
     * @brief Logical end: equal if same physical position, predicate false on `*L`, or both at physical end.
     * @warning Do not call `pred` on `rhs`’s position; `end` uses the same `pred` instance as `begin`.
     */
    [[nodiscard]] bool operator==(take_while_iterator const& rhs) const
    {
        BaseIt const& L = static_cast<BaseIt const&>(*this);
        BaseIt const& R = static_cast<BaseIt const&>(rhs);
        if (L == R)
            return true;
        if (!static_cast<pred_holder const&>(*this).fn()(*L))
            return true;
        return false;
    }

    /** @brief Negation of `operator==`. */
    [[nodiscard]] bool operator!=(take_while_iterator const& rhs) const { return !(*this == rhs); }
    /** @} */
};

/**
 * @ingroup linq
 * @brief Takes at most `N` elements from the base range (`Take`); `remaining` pairs with end sentinel.
 * @tparam BaseIt Source iterator.
 * @details `remaining_` counts consumed elements; end iterator uses `remaining == 0`. Negative `take` counts
 *          are normalized in the view (`take_n_view`), not here. After the final yield, `operator++` reaches
 *          `remaining_ == 0` **without** advancing the base iterator again, so filtered tails are not traversed.
 */
template <class BaseIt>
class take_n_iterator : public BaseIt {
public:
    using iterator_category = detail::clamp_iterator_category_t<
        typename std::iterator_traits<BaseIt>::iterator_category>;
    using value_type = typename std::iterator_traits<BaseIt>::value_type;
    using difference_type = typename std::iterator_traits<BaseIt>::difference_type;
    using pointer = typename std::iterator_traits<BaseIt>::pointer;
    using reference = typename std::iterator_traits<BaseIt>::reference;

    /** @name Construction (take_n_iterator) */
    /** @{ */
    take_n_iterator() = delete;
    take_n_iterator(take_n_iterator const&) = default;
    take_n_iterator& operator=(take_n_iterator const&) = default;

    /**
     * @brief Pairs `base` with `remaining` budget (see `take_n_view` for sign convention).
     * @param base Current underlying position.
     * @param remaining Negated take budget at begin (`take_n_view` passes `-count`); `0` at end iterator.
     */
    take_n_iterator(BaseIt base, std::ptrdiff_t remaining) noexcept(
        std::is_nothrow_move_constructible_v<BaseIt>)
        : BaseIt(std::move(base)), remaining_(remaining)
    {}
    /** @} */

    /** @name Traversal */
    /** @{ */
    /**
     * @brief Step toward logical end: bump `remaining_` toward `0`; advance base only while more yields remain.
     * @details When `remaining_` reaches `0`, the base position is **not** advanced further — the logical range is
     *          exhausted (`operator==` matches `end` via `remaining_`). That avoids walking the rest of a filtered
     *          underlying sequence (e.g. `where` predicates) after the last taken element.
     */
    take_n_iterator& operator++() noexcept(noexcept(++std::declval<BaseIt&>()))
    {
        ++remaining_;
        if (remaining_ < 0)
            ++static_cast<BaseIt&>(*this);
        return *this;
    }

    /** @brief Post-increment. */
    take_n_iterator operator++(int)
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    /** @brief Undo one step: decrement `remaining_` and retreat base. */
    take_n_iterator& operator--() noexcept(noexcept(--std::declval<BaseIt&>()))
    {
        --remaining_;
        --static_cast<BaseIt&>(*this);
        return *this;
    }

    /** @brief Post-decrement. */
    take_n_iterator operator--(int)
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }
    /** @} */

    /** @name Comparison */
    /** @{ */
    /** @brief End when `remaining_` matches or underlying iterators match (see `take_n_view`). */
    [[nodiscard]] friend bool operator!=(take_n_iterator const& a, take_n_iterator const& b) noexcept
    {
        return static_cast<BaseIt const&>(a) != static_cast<BaseIt const&>(b) && a.remaining_ != b.remaining_;
    }

    /** @brief True if same underlying position or same `remaining_` (paired end sentinels). */
    [[nodiscard]] friend bool operator==(take_n_iterator const& a, take_n_iterator const& b) noexcept
    {
        return static_cast<BaseIt const&>(a) == static_cast<BaseIt const&>(b) || a.remaining_ == b.remaining_;
    }
    /** @} */

private:
    std::ptrdiff_t remaining_;
};

} // namespace qb::linq

#undef QB_LINQ_ITER_EMPTY_BASES

/**
 * @brief Ensure `std::iterator_traits` reports the clamped category for adaptors that inherit a random-access
 *        base (`operator-` still exists on the base subobject; libstdc++/MSVC must not treat them as random-access).
 */
namespace std {

template <class BaseIt, class Pred>
struct iterator_traits<::qb::linq::where_iterator<BaseIt, Pred>> {
    using iterator_category =
        ::qb::linq::detail::clamp_iterator_category_t<typename iterator_traits<BaseIt>::iterator_category>;
    using value_type = typename ::qb::linq::where_iterator<BaseIt, Pred>::value_type;
    using difference_type = typename ::qb::linq::where_iterator<BaseIt, Pred>::difference_type;
    using pointer = typename ::qb::linq::where_iterator<BaseIt, Pred>::pointer;
    using reference = typename ::qb::linq::where_iterator<BaseIt, Pred>::reference;
};

template <class BaseIt, class Pred>
struct iterator_traits<::qb::linq::take_while_iterator<BaseIt, Pred>> {
    using iterator_category =
        ::qb::linq::detail::clamp_iterator_category_t<typename iterator_traits<BaseIt>::iterator_category>;
    using value_type = typename ::qb::linq::take_while_iterator<BaseIt, Pred>::value_type;
    using difference_type = typename ::qb::linq::take_while_iterator<BaseIt, Pred>::difference_type;
    using pointer = typename ::qb::linq::take_while_iterator<BaseIt, Pred>::pointer;
    using reference = typename ::qb::linq::take_while_iterator<BaseIt, Pred>::reference;
};

template <class BaseIt>
struct iterator_traits<::qb::linq::take_n_iterator<BaseIt>> {
    using iterator_category =
        ::qb::linq::detail::clamp_iterator_category_t<typename iterator_traits<BaseIt>::iterator_category>;
    using value_type = typename ::qb::linq::take_n_iterator<BaseIt>::value_type;
    using difference_type = typename ::qb::linq::take_n_iterator<BaseIt>::difference_type;
    using pointer = typename ::qb::linq::take_n_iterator<BaseIt>::pointer;
    using reference = typename ::qb::linq::take_n_iterator<BaseIt>::reference;
};

} // namespace std

// ---- qb/linq/reverse_iterator.h ----

/**
 * @file reverse_iterator.h
 * @ingroup linq
 * @brief `qb::linq::reverse_iterator` — same behavior as `std::reverse_iterator` without requiring a
 *        default-constructible base iterator.
 *
 * @details
 * Stores an explicit base position `current` (same convention as `std::reverse_iterator::base()`).
 * There is **no default constructor**. Random-access operations (`+=`, `+`, `-`, `[]`, iterator difference)
 * are available only when the base iterator is random-access.
 */


namespace qb::linq {

template <class Iterator>
class reverse_iterator {
    static_assert(std::is_base_of_v<std::bidirectional_iterator_tag,
                      typename std::iterator_traits<Iterator>::iterator_category>,
        "qb::linq::reverse_iterator requires a bidirectional (or stronger) base iterator");

    template <class U>
    using enable_if_ra_base = std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag,
        typename std::iterator_traits<U>::iterator_category>>;

    Iterator current_;

public:
    using iterator_type = Iterator;
    using iterator_category = typename std::iterator_traits<Iterator>::iterator_category;
    using value_type = typename std::iterator_traits<Iterator>::value_type;
    using difference_type = typename std::iterator_traits<Iterator>::difference_type;
    using pointer = typename std::iterator_traits<Iterator>::pointer;
    using reference = typename std::iterator_traits<Iterator>::reference;

    reverse_iterator() = delete;

    explicit reverse_iterator(Iterator x) noexcept(std::is_nothrow_move_constructible_v<Iterator>)
        : current_(std::move(x))
    {}

    reverse_iterator(reverse_iterator const&) = default;
    reverse_iterator(reverse_iterator&&) noexcept(std::is_nothrow_move_constructible_v<Iterator>) = default;
    reverse_iterator& operator=(reverse_iterator const&) = default;
    reverse_iterator& operator=(reverse_iterator&&) noexcept(
        std::is_nothrow_move_assignable_v<Iterator>) = default;

    [[nodiscard]] Iterator const& base() const noexcept { return current_; }

    [[nodiscard]] reference operator*() const
    {
        Iterator tmp = current_;
        return *--tmp;
    }

    [[nodiscard]] pointer operator->() const
    {
        Iterator tmp = current_;
        --tmp;
        return std::addressof(*tmp);
    }

    reverse_iterator& operator++() noexcept(noexcept(--std::declval<Iterator&>()))
    {
        --current_;
        return *this;
    }

    reverse_iterator operator++(int)
    {
        reverse_iterator tmp(*this);
        --current_;
        return tmp;
    }

    reverse_iterator& operator--() noexcept(noexcept(++std::declval<Iterator&>()))
    {
        ++current_;
        return *this;
    }

    reverse_iterator operator--(int)
    {
        reverse_iterator tmp(*this);
        ++current_;
        return tmp;
    }

    [[nodiscard]] friend bool operator==(reverse_iterator const& a, reverse_iterator const& b) noexcept(
        noexcept(a.current_ == b.current_))
    {
        return a.current_ == b.current_;
    }

    [[nodiscard]] friend bool operator!=(reverse_iterator const& a, reverse_iterator const& b) noexcept(
        noexcept(a.current_ != b.current_))
    {
        return a.current_ != b.current_;
    }

    template <typename BaseIt = Iterator, typename = enable_if_ra_base<BaseIt>>
    reverse_iterator& operator+=(difference_type n) noexcept(noexcept(current_ -= n))
    {
        current_ -= n;
        return *this;
    }

    template <typename BaseIt = Iterator, typename = enable_if_ra_base<BaseIt>>
    [[nodiscard]] reverse_iterator operator+(difference_type n) const
        noexcept(noexcept(Iterator(std::declval<Iterator const&>() - n)))
    {
        return reverse_iterator(current_ - n);
    }

    template <typename BaseIt = Iterator, typename = enable_if_ra_base<BaseIt>>
    reverse_iterator& operator-=(difference_type n) noexcept(noexcept(current_ += n))
    {
        current_ += n;
        return *this;
    }

    template <typename BaseIt = Iterator, typename = enable_if_ra_base<BaseIt>>
    [[nodiscard]] reverse_iterator operator-(difference_type n) const
        noexcept(noexcept(Iterator(std::declval<Iterator const&>() + n)))
    {
        return reverse_iterator(current_ + n);
    }

    template <typename BaseIt = Iterator, typename = enable_if_ra_base<BaseIt>>
    [[nodiscard]] reference operator[](difference_type n) const
    {
        return *(*this + n);
    }
};

template <class Iterator>
std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag,
                      typename std::iterator_traits<Iterator>::iterator_category>,
    reverse_iterator<Iterator>>
operator+(typename reverse_iterator<Iterator>::difference_type n, reverse_iterator<Iterator> const& x) noexcept(
    noexcept(x + n))
{
    return x + n;
}

template <class Iterator>
std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag,
                      typename std::iterator_traits<Iterator>::iterator_category>,
    typename reverse_iterator<Iterator>::difference_type>
operator-(reverse_iterator<Iterator> const& lhs, reverse_iterator<Iterator> const& rhs) noexcept(
    noexcept(rhs.base() - lhs.base()))
{
    return rhs.base() - lhs.base();
}

} // namespace qb::linq

namespace std {

template <class Iterator>
struct iterator_traits<::qb::linq::reverse_iterator<Iterator>> {
    using iterator_category = typename ::qb::linq::reverse_iterator<Iterator>::iterator_category;
    using value_type = typename ::qb::linq::reverse_iterator<Iterator>::value_type;
    using difference_type = typename ::qb::linq::reverse_iterator<Iterator>::difference_type;
    using pointer = typename ::qb::linq::reverse_iterator<Iterator>::pointer;
    using reference = typename ::qb::linq::reverse_iterator<Iterator>::reference;
};

} // namespace std

// ---- qb/linq/detail/extra_views.h ----

/**
 * @file extra_views.h
 * @ingroup linq
 * @brief Lazy view implementations used by `query_range_algorithms` (`concat`, `zip`, `distinct`, `scan`, …).
 *
 * @details
 * Declarations of `concat`, `zip`, `scan`, etc. live in `query_range.h`; definitions that need complete view
 * types are placed here and included from `query.h` after `query_range_algorithms` is declared.
 *
 * @par Iterator / view contracts
 * - **empty_view / single_view / repeat_view:** forward iterators; factory helpers `empty<T>()`, `once`,
 *   `repeat` in `enumerable.h`.
 * - **concat_view / zip_view:** forward iterators; both ranges must share compatible element types for
 *   `concat`; `zip` yields `std::pair` references until one side exhausts, then canonical end.
 * - **default_if_empty_view:** if the source `[b,e)` is empty, yields `def` once; otherwise forwards.
 * - **distinct_view:** first occurrence per key (`unordered_set`); fresh set on each `begin()` for correct
 *   re-enumeration.
 * - **enumerate_view:** `(index, element)` pairs; index starts at 0.
 * - **chunk_view:** each `operator*` copies up to `n` elements into an internal `std::vector` (lazy per chunk).
 * - **stride_view:** step clamped to at least 1.
 * - **scan_view:** yields running `f(acc, x)`; empty source → no values; `F` stored in the view; iterators hold a
 *   non-owning `F*` (no refcount); iterators must not outlive the `scan_view` they came from.
 *
 * @par Types in this header
 * | Iterator / view | Role |
 * |-----------------|------|
 * | `empty_iterator` / `empty_view` | Always-empty range |
 * | `single_iterator` / `single_view` | Exactly one element |
 * | `repeat_iterator` / `repeat_view` | `n` copies of one value |
 * | `concat_iterator` / `concat_view` | Two ranges back-to-back |
 * | `zip_iterator` / `zip_view` | Pairwise tuples until shorter side ends |
 * | `zip3_iterator` / `zip3_view` | Three-way tuples until shortest side ends |
 * | `default_if_empty_iterator` / `default_if_empty_view` | Default value if source empty |
 * | `distinct_iterator` / `distinct_view` | First occurrence per key (hash set) |
 * | `enumerate_iterator` / `enumerate_view` | `(index, element)` |
 * | `chunk_iterator` / `chunk_view` | Slices as `std::vector` chunks |
 * | `stride_iterator` / `stride_view` | Every k-th element |
 * | `scan_iterator` / `scan_view` | Inclusive prefix fold |
 */


namespace qb::linq {

namespace detail {

// ---------------------------------------------------------------------------
// empty_view
// ---------------------------------------------------------------------------

/**
 * @ingroup linq
 * @brief Sentinel forward iterator for an always-empty range (`operator*` returns default-constructed `T`).
 */
template <class T>
class empty_iterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::remove_cv_t<T>;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;

    /** @brief Dereference is only valid for exposition; value is default-constructed `T`. */
    [[nodiscard]] reference operator*() const { return value_type{}; }
    /** @brief No-op (empty range). */
    empty_iterator& operator++() { return *this; }
    /** @brief No-op. */
    empty_iterator operator++(int) { return *this; }
    /** @brief All empty iterators compare equal. */
    [[nodiscard]] friend bool operator==(empty_iterator const&, empty_iterator const&) noexcept { return true; }
    [[nodiscard]] friend bool operator!=(empty_iterator const& a, empty_iterator const& b) noexcept
    {
        return !(a == b);
    }
};

/**
 * @ingroup linq
 * @brief Lazy empty sequence of `T` (LINQ `Empty`).
 */
template <class T>
class empty_view : public query_range_algorithms<empty_view<T>, empty_iterator<T>> {
public:
    /** @brief Begin of empty range. */
    [[nodiscard]] empty_iterator<T> begin() const noexcept { return {}; }
    /** @brief End (same as begin). */
    [[nodiscard]] empty_iterator<T> end() const noexcept { return {}; }
};

// ---------------------------------------------------------------------------
// single_view
// ---------------------------------------------------------------------------

/**
 * @ingroup linq
 * @brief Yields one value then reaches end (`step` 0 → begin, 1 → end).
 */
template <class T>
class single_iterator {
    int step_{0};
    T val_{};

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::remove_cv_t<T>;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;

    /** @brief Value-initialized; pair with `end` only in valid ranges. */
    single_iterator() = default;
    /**
     * @brief `step == 0` → begin position; `step == 1` → past-the-end.
     */
    explicit single_iterator(T v, int step) noexcept(std::is_nothrow_move_constructible_v<T>)
        : step_(step), val_(std::move(v))
    {}

    /** @brief The stored single value. */
    [[nodiscard]] reference operator*() const noexcept { return val_; }

    /** @brief Moves to past-the-end (`step_` becomes `1`). */
    single_iterator& operator++() noexcept
    {
        ++step_;
        return *this;
    }

    /** @brief Post-increment. */
    single_iterator operator++(int) noexcept
    {
        auto t = *this;
        ++*this;
        return t;
    }

    /** @brief Equality on `step_` only. */
    [[nodiscard]] friend bool operator==(single_iterator const& a, single_iterator const& b) noexcept
    {
        return a.step_ == b.step_;
    }
    /** @brief Inequality. */
    [[nodiscard]] friend bool operator!=(single_iterator const& a, single_iterator const& b) noexcept
    {
        return !(a == b);
    }
};

/**
 * @ingroup linq
 * @brief Exactly one element (used by `once` and `append`/`prepend`).
 */
template <class T>
class single_view : public query_range_algorithms<single_view<T>, single_iterator<T>> {
    T val_{};

public:
    /** @param v Element to yield once. */
    explicit single_view(T v) noexcept(std::is_nothrow_move_constructible_v<T>) : val_(std::move(v)) {}

    /** @brief Iterator before the single element. */
    [[nodiscard]] single_iterator<T> begin() const { return single_iterator<T>(val_, 0); }
    /** @brief Past-the-end after one element. */
    [[nodiscard]] single_iterator<T> end() const { return single_iterator<T>(val_, 1); }
};

// ---------------------------------------------------------------------------
// repeat_view
// ---------------------------------------------------------------------------

/**
 * @ingroup linq
 * @brief Counted repetition iterator (`i` from `0` to `n`).
 */
template <class T>
class repeat_iterator {
    T val_{};
    std::size_t i_{};
    std::size_t n_{};

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::remove_cv_t<T>;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;

    /** @brief Default; use with paired `end` only in valid ranges. */
    repeat_iterator() = default;
    /**
     * @brief Current index `i` in `[0, n]`; `i == n` is past-the-end.
     */
    repeat_iterator(T v, std::size_t i, std::size_t n) noexcept(std::is_nothrow_move_constructible_v<T>)
        : val_(std::move(v)), i_(i), n_(n)
    {}

    /** @brief Repeated value (by value). */
    [[nodiscard]] reference operator*() const noexcept { return val_; }

    /** @brief Advance index toward `n`. */
    repeat_iterator& operator++() noexcept
    {
        ++i_;
        return *this;
    }

    /** @brief Post-increment. */
    repeat_iterator operator++(int) noexcept
    {
        auto t = *this;
        ++*this;
        return t;
    }

    /** @brief Equality on index `i_`. */
    [[nodiscard]] friend bool operator==(repeat_iterator const& a, repeat_iterator const& b) noexcept
    {
        return a.i_ == b.i_;
    }
    /** @brief Inequality. */
    [[nodiscard]] friend bool operator!=(repeat_iterator const& a, repeat_iterator const& b) noexcept
    {
        return !(a == b);
    }
};

/**
 * @ingroup linq
 * @brief Repeats the same value `n` times (LINQ `Repeat`).
 */
template <class T>
class repeat_view : public query_range_algorithms<repeat_view<T>, repeat_iterator<T>> {
    T val_{};
    std::size_t n_{};

public:
    /**
     * @param v Repeated value.
     * @param n Number of yields (`0` → empty range).
     */
    repeat_view(T v, std::size_t n) noexcept(std::is_nothrow_move_constructible_v<T>)
        : val_(std::move(v)), n_(n)
    {}

    /** @brief Iterator at first repeat. */
    [[nodiscard]] repeat_iterator<T> begin() const { return repeat_iterator<T>(val_, 0, n_); }
    /** @brief Past-the-end when `i == n`. */
    [[nodiscard]] repeat_iterator<T> end() const { return repeat_iterator<T>(val_, n_, n_); }
};

// ---------------------------------------------------------------------------
// concat_view
// ---------------------------------------------------------------------------

/**
 * @ingroup linq
 * @brief Forward iterator over `[a_begin,a_end)` then `[b_begin,b_end)`.
 * @tparam It1 First range iterator.
 * @tparam It2 Second range iterator; `value_type` must match `It1` (after removing cv).
 * @details `reference` is the conditional-operator common type of `*It1` and `*It2` so mixed `T&` / `T const&`
 *          legs (e.g. `union_with` on a `const` right-hand range) remain valid.
 */
template <class It1, class It2>
class concat_iterator {
    It1 a_{}, a_end_{};
    It2 b_{}, b_end_{};
    bool in_first_{true};

public:
    static_assert(std::is_same_v<std::remove_cv_t<typename std::iterator_traits<It1>::value_type>,
                      std::remove_cv_t<typename std::iterator_traits<It2>::value_type>>,
        "qb::linq::concat requires the same element type on both sides");

    using iterator_category = std::forward_iterator_tag;
    using value_type = typename std::iterator_traits<It1>::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    /** @brief Common reference of both legs (`int&` + `const int&` → `const int&`); not only `It1::reference`. */
    using reference = decltype(false ? *std::declval<It1&>() : *std::declval<It2&>());

    /** @brief Default; for end-sentinel construction use `is_end == true`. */
    concat_iterator() = default;

    /**
     * @brief Positions over first leg then second; `is_end` builds canonical past-the-end.
     * @param ab First range begin.
     * @param ae First range end.
     * @param bb Second range begin.
     * @param be Second range end.
     * @param is_end If true, past-the-end (both legs exhausted).
     */
    concat_iterator(It1 ab, It1 ae, It2 bb, It2 be, bool is_end)
        : a_(std::move(ab)), a_end_(std::move(ae)), b_(std::move(bb)), b_end_(std::move(be))
    {
        if (is_end) {
            in_first_ = false;
            a_ = a_end_;
            b_ = b_end_;
        } else if (a_ == a_end_) {
            in_first_ = false;
        }
    }

    /** @brief Dereference current leg (`in_first_` selects `a_` vs `b_`). */
    [[nodiscard]] reference operator*() const { return in_first_ ? *a_ : *b_; }

    /** @brief Advance in first leg until exhausted, then in second. */
    concat_iterator& operator++()
    {
        // Past-the-end: both legs exhausted (including `end()` sentinel) — do not increment past `b_end_`.
        if (!in_first_ && a_ == a_end_ && b_ == b_end_)
            return *this;
        if (in_first_) {
            ++a_;
            if (a_ == a_end_)
                in_first_ = false;
        } else {
            ++b_;
        }
        return *this;
    }

    /** @brief Post-increment. */
    concat_iterator operator++(int)
    {
        auto t = *this;
        ++*this;
        return t;
    }

    /** @brief Equality on leg and both cursors. */
    [[nodiscard]] friend bool operator==(concat_iterator const& x, concat_iterator const& y) noexcept
    {
        return x.in_first_ == y.in_first_ && x.a_ == y.a_ && x.b_ == y.b_;
    }
    [[nodiscard]] friend bool operator!=(concat_iterator const& x, concat_iterator const& y) noexcept
    {
        return !(x == y);
    }
};

/**
 * @ingroup linq
 * @brief Lazy concatenation of two iterator ranges (`Concat`).
 */
template <class It1, class It2>
class concat_view : public query_range_algorithms<concat_view<It1, It2>, concat_iterator<It1, It2>> {
    It1 a0_{}, a1_{};
    It2 b0_{}, b1_{};

public:
    /**
     * @param ab First begin.
     * @param ae First end.
     * @param bb Second begin.
     * @param be Second end (`value_type` must match first leg).
     */
    concat_view(It1 ab, It1 ae, It2 bb, It2 be)
        : a0_(std::move(ab)), a1_(std::move(ae)), b0_(std::move(bb)), b1_(std::move(be))
    {}

    /** @brief Start of concatenated range (first leg, or second if first empty). */
    [[nodiscard]] concat_iterator<It1, It2> begin() const
    {
        return concat_iterator<It1, It2>(a0_, a1_, b0_, b1_, false);
    }
    /** @brief Past-the-end of full concatenation. */
    [[nodiscard]] concat_iterator<It1, It2> end() const
    {
        return concat_iterator<It1, It2>(a1_, a1_, b1_, b1_, true);
    }
};

// ---------------------------------------------------------------------------
// zip_view
// ---------------------------------------------------------------------------

/**
 * @ingroup linq
 * @brief Pairs elements from two forward sequences; `operator++` advances both while neither is exhausted.
 * @details When either side reaches its end, both cursors jump to `(ae, be)` (canonical end).
 */
template <class It1, class It2>
class zip_iterator {
    It1 a_{};
    It2 b_{};
    It1 ae_{};
    It2 be_{};

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<typename std::iterator_traits<It1>::value_type,
        typename std::iterator_traits<It2>::value_type>;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = std::pair<typename std::iterator_traits<It1>::reference,
        typename std::iterator_traits<It2>::reference>;

    /** @brief Default; canonical end uses `(ae, be, ae, be)`. */
    zip_iterator() = default;
    /**
     * @brief Holds both cursors and both ends.
     * @param a Left current iterator.
     * @param b Right current iterator.
     * @param ae Left end.
     * @param be Right end.
     */
    zip_iterator(It1 a, It2 b, It1 ae, It2 be)
        : a_(std::move(a)), b_(std::move(b)), ae_(std::move(ae)), be_(std::move(be))
    {}

    /** @brief Pair of references `{*a_, *b_}`. */
    [[nodiscard]] reference operator*() const { return reference{*a_, *b_}; }

    /**
     * @brief Increments both legs; if either hits its end, both cursors jump to `(ae, be)` (canonical end).
     */
    zip_iterator& operator++()
    {
        if (a_ == ae_ && b_ == be_)
            return *this;
        if (a_ != ae_)
            ++a_;
        if (b_ != be_)
            ++b_;
        if (a_ == ae_ || b_ == be_) {
            a_ = ae_;
            b_ = be_;
        }
        return *this;
    }

    /** @brief Post-increment. */
    zip_iterator operator++(int)
    {
        auto t = *this;
        ++*this;
        return t;
    }

    /** @brief Equality on both cursors. */
    [[nodiscard]] friend bool operator==(zip_iterator const& x, zip_iterator const& y) noexcept
    {
        return x.a_ == y.a_ && x.b_ == y.b_;
    }
    [[nodiscard]] friend bool operator!=(zip_iterator const& x, zip_iterator const& y) noexcept
    {
        return !(x == y);
    }
};

/**
 * @ingroup linq
 * @brief Lazy zip of two ranges (`Zip`).
 */
template <class It1, class It2>
class zip_view : public query_range_algorithms<zip_view<It1, It2>, zip_iterator<It1, It2>> {
    It1 a0_{}, a1_{};
    It2 b0_{}, b1_{};

public:
    /**
     * @param ab Left begin.
     * @param ae Left end.
     * @param bb Right begin.
     * @param be Right end.
     */
    zip_view(It1 ab, It1 ae, It2 bb, It2 be)
        : a0_(std::move(ab)), a1_(std::move(ae)), b0_(std::move(bb)), b1_(std::move(be))
    {}

    /**
     * @brief Empty if either side empty; otherwise length is `min(|left|, |right|)` (remainder ignored).
     */
    [[nodiscard]] zip_iterator<It1, It2> begin() const
    {
        if (a0_ == a1_ || b0_ == b1_)
            return {a1_, b1_, a1_, b1_};
        return {a0_, b0_, a1_, b1_};
    }
    /** @brief Canonical end `(a1_, b1_, a1_, b1_)`. */
    [[nodiscard]] zip_iterator<It1, It2> end() const { return {a1_, b1_, a1_, b1_}; }
};

// ---------------------------------------------------------------------------
// zip3_view — three sequences in lockstep
// ---------------------------------------------------------------------------

/**
 * @ingroup linq
 * @brief Triple zip: advances three cursors; canonical end when any leg exhausts.
 */
template <class It1, class It2, class It3>
class zip3_iterator {
    It1 a_{};
    It2 b_{};
    It3 c_{};
    It1 ae_{};
    It2 be_{};
    It3 ce_{};

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::tuple<typename std::iterator_traits<It1>::value_type,
        typename std::iterator_traits<It2>::value_type, typename std::iterator_traits<It3>::value_type>;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = std::tuple<typename std::iterator_traits<It1>::reference,
        typename std::iterator_traits<It2>::reference, typename std::iterator_traits<It3>::reference>;

    zip3_iterator() = default;

    zip3_iterator(It1 a, It2 b, It3 c, It1 ae, It2 be, It3 ce)
        : a_(std::move(a)), b_(std::move(b)), c_(std::move(c)), ae_(std::move(ae)), be_(std::move(be)), ce_(std::move(ce))
    {}

    [[nodiscard]] reference operator*() const { return reference{*a_, *b_, *c_}; }

    zip3_iterator& operator++()
    {
        if (a_ == ae_ && b_ == be_ && c_ == ce_)
            return *this;
        if (a_ != ae_)
            ++a_;
        if (b_ != be_)
            ++b_;
        if (c_ != ce_)
            ++c_;
        if (a_ == ae_ || b_ == be_ || c_ == ce_) {
            a_ = ae_;
            b_ = be_;
            c_ = ce_;
        }
        return *this;
    }

    zip3_iterator operator++(int)
    {
        auto t = *this;
        ++*this;
        return t;
    }

    [[nodiscard]] friend bool operator==(zip3_iterator const& x, zip3_iterator const& y) noexcept
    {
        return x.a_ == y.a_ && x.b_ == y.b_ && x.c_ == y.c_;
    }
    [[nodiscard]] friend bool operator!=(zip3_iterator const& x, zip3_iterator const& y) noexcept
    {
        return !(x == y);
    }
};

/**
 * @ingroup linq
 * @brief Lazy zip of three ranges.
 */
template <class It1, class It2, class It3>
class zip3_view : public query_range_algorithms<zip3_view<It1, It2, It3>, zip3_iterator<It1, It2, It3>> {
    It1 a0_{}, a1_{};
    It2 b0_{}, b1_{};
    It3 c0_{}, c1_{};

public:
    zip3_view(It1 ab, It1 ae, It2 bb, It2 be, It3 cb, It3 ce)
        : a0_(std::move(ab)), a1_(std::move(ae)), b0_(std::move(bb)), b1_(std::move(be)), c0_(std::move(cb)),
          c1_(std::move(ce))
    {}

    [[nodiscard]] zip3_iterator<It1, It2, It3> begin() const
    {
        if (a0_ == a1_ || b0_ == b1_ || c0_ == c1_)
            return {a1_, b1_, c1_, a1_, b1_, c1_};
        return {a0_, b0_, c0_, a1_, b1_, c1_};
    }

    [[nodiscard]] zip3_iterator<It1, It2, It3> end() const { return {a1_, b1_, c1_, a1_, b1_, c1_}; }
};

// ---------------------------------------------------------------------------
// default_if_empty_view — one \p def when [\p b,\p e) is empty
// ---------------------------------------------------------------------------

/** @brief Forward declaration (iterator is a friend). */
template <class It, class T>
class default_if_empty_view;

/**
 * @ingroup linq
 * @brief Iterator over source range or a single synthetic default when the range was empty at construction.
 * @tparam It Underlying iterator; `T` must match `iterator_traits<It>::value_type` (modulo cv).
 */
template <class It, class T>
class default_if_empty_iterator {
    default_if_empty_view<It, T> const* parent_{nullptr};
    It cur_{};
    It end_{};
    /** @brief `0` walk source, `1` yield `def` once, `2` done. */
    unsigned char phase_{0};

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::remove_cv_t<typename std::iterator_traits<It>::value_type>;
    static_assert(std::is_same_v<value_type, std::remove_cv_t<T>>,
        "qb::linq::default_if_empty: default must match sequence value_type");
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = typename std::iterator_traits<It>::reference;

    default_if_empty_iterator() = default;

    /**
     * @param parent Owning view (for `def` in phase 1).
     * @param b Source begin.
     * @param e Source end.
     * @param past_end End sentinel iterator state.
     */
    default_if_empty_iterator(
        default_if_empty_view<It, T> const* parent, It b, It e, bool past_end)
        : parent_(parent), cur_(std::move(b)), end_(std::move(e))
    {
        if (past_end) {
            phase_ = 2;
            cur_ = end_;
        } else if (cur_ == end_) {
            phase_ = 1;
        } else {
            phase_ = 0;
        }
    }

    /** @brief Source element or synthetic default in phase 1. */
    [[nodiscard]] reference operator*() const
    {
        if (phase_ == 1)
            return parent_->default_value_ref();
        return *cur_;
    }

    /** @brief Advance source or leave default phase. */
    default_if_empty_iterator& operator++()
    {
        if (phase_ == 0) {
            ++cur_;
            if (cur_ == end_)
                phase_ = 2;
        } else if (phase_ == 1) {
            phase_ = 2;
        }
        return *this;
    }

    /** @brief Post-increment. */
    default_if_empty_iterator operator++(int)
    {
        auto t = *this;
        ++*this;
        return t;
    }

    /** @brief Equality on `phase_` and `cur_`. */
    [[nodiscard]] friend bool operator==(default_if_empty_iterator const& x, default_if_empty_iterator const& y) noexcept
    {
        return x.phase_ == y.phase_ && x.cur_ == y.cur_;
    }
    [[nodiscard]] friend bool operator!=(default_if_empty_iterator const& x, default_if_empty_iterator const& y) noexcept
    {
        return !(x == y);
    }
};

/**
 * @ingroup linq
 * @brief `DefaultIfEmpty` — substitutes one value when `[b,e)` is empty.
 */
template <class It, class T>
class default_if_empty_view : public query_range_algorithms<default_if_empty_view<It, T>, default_if_empty_iterator<It, T>> {
    friend class default_if_empty_iterator<It, T>;

    It b_{}, e_{};
    mutable T def_{};

    [[nodiscard]] T& default_value_ref() const noexcept { return def_; }

public:
    /**
     * @param b Source begin.
     * @param e Source end.
     * @param d Value yielded once when `[b,e)` is empty.
     */
    default_if_empty_view(It b, It e, T d) : b_(std::move(b)), e_(std::move(e)), def_(std::move(d)) {}

    /** @brief Walk source or single default if initially empty. */
    [[nodiscard]] default_if_empty_iterator<It, T> begin() const
    {
        return default_if_empty_iterator<It, T>(this, b_, e_, false);
    }
    /** @brief Past-the-end (including after default phase). */
    [[nodiscard]] default_if_empty_iterator<It, T> end() const
    {
        return default_if_empty_iterator<It, T>(this, b_, e_, true);
    }
};

// ---------------------------------------------------------------------------
// distinct_view — first occurrence of each key (hash)
// ---------------------------------------------------------------------------

/**
 * @ingroup linq
 * @brief Skips elements whose key was already seen in the shared `unordered_set`.
 */
template <class BaseIt, class KeyFn, class KeyT>
class distinct_iterator {
    BaseIt cur_{};
    BaseIt end_{};
    KeyFn keyfn_{};
    std::shared_ptr<std::unordered_set<KeyT>> seen_{};

    void skip_seen()
    {
        if (!seen_)
            return;
        while (cur_ != end_) {
            KeyT const k = keyfn_(*cur_);
            auto const [_, ins] = seen_->insert(k);
            if (ins)
                break;
            ++cur_;
        }
    }

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename std::iterator_traits<BaseIt>::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = typename std::iterator_traits<BaseIt>::reference;

    distinct_iterator() = default;

    /**
     * @param b Range begin.
     * @param e Range end.
     * @param kf Key function for deduplication.
     * @param s Shared set (`nullptr` or empty for end iterator).
     * @param is_end End position if true.
     */
    distinct_iterator(
        BaseIt b, BaseIt e, KeyFn kf, std::shared_ptr<std::unordered_set<KeyT>> s, bool is_end)
        : cur_(std::move(b)), end_(std::move(e)), keyfn_(std::move(kf)), seen_(std::move(s))
    {
        if (!is_end && seen_)
            skip_seen();
        if (is_end) {
            cur_ = end_;
        }
    }

    /** @brief Current element (already passed key filter). */
    [[nodiscard]] reference operator*() const { return *cur_; }

    /** @brief Next unseen key. */
    distinct_iterator& operator++()
    {
        ++cur_;
        skip_seen();
        return *this;
    }

    /** @brief Post-increment. */
    distinct_iterator operator++(int)
    {
        auto t = *this;
        ++*this;
        return t;
    }

    [[nodiscard]] friend bool operator==(distinct_iterator const& x, distinct_iterator const& y) noexcept
    {
        return x.cur_ == y.cur_;
    }
    [[nodiscard]] friend bool operator!=(distinct_iterator const& x, distinct_iterator const& y) noexcept
    {
        return !(x == y);
    }
};

/**
 * @ingroup linq
 * @brief Distinct elements by key (`DistinctBy`); `distinct()` uses `identity_proj`.
 * @note A new `unordered_set` is allocated on each `begin()` so repeated full traversals stay correct.
 */
template <class BaseIt, class KeyFn, class KeyT>
class distinct_view
    : public query_range_algorithms<distinct_view<BaseIt, KeyFn, KeyT>, distinct_iterator<BaseIt, KeyFn, KeyT>> {
    BaseIt b_{}, e_{};
    KeyFn keyfn_{};

public:
    /**
     * @param b Source begin.
     * @param e Source end.
     * @param kf Key extractor (`KeyT` must be hashable / equality-comparable for `unordered_set`).
     */
    distinct_view(BaseIt b, BaseIt e, KeyFn kf)
        : b_(std::move(b)), e_(std::move(e)), keyfn_(std::move(kf))
    {}

    /** @brief New `unordered_set` per `begin()` so full re-walks see all keys again. */
    [[nodiscard]] distinct_iterator<BaseIt, KeyFn, KeyT> begin() const
    {
        auto s = std::make_shared<std::unordered_set<KeyT>>();
        reserve_if_random_access(*s, b_, e_);
        return distinct_iterator<BaseIt, KeyFn, KeyT>(b_, e_, keyfn_, std::move(s), false);
    }
    [[nodiscard]] distinct_iterator<BaseIt, KeyFn, KeyT> end() const
    {
        return distinct_iterator<BaseIt, KeyFn, KeyT>(e_, e_, keyfn_, {}, true);
    }
};

// ---------------------------------------------------------------------------
// enumerate_view — (index, element) pairs; lazy (LINQ overload with index).
// ---------------------------------------------------------------------------

/**
 * @ingroup linq
 * @brief `operator*` returns `{n, *it}`; `reference` is a pair of index and base reference.
 */
template <class BaseIt>
class enumerate_iterator {
    BaseIt it_{};
    std::size_t n_{};

public:
    using iterator_category = std::forward_iterator_tag;
    using base_ref = typename std::iterator_traits<BaseIt>::reference;
    using value_type =
        std::pair<std::size_t, std::remove_cv_t<typename std::iterator_traits<BaseIt>::value_type>>;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = std::pair<std::size_t, base_ref>;

    enumerate_iterator() = default;
    /** @brief Iterator at `it` with running index `n`. */
    enumerate_iterator(BaseIt it, std::size_t n) : it_(std::move(it)), n_(n) {}

    /** @brief `{index, *it}` pair. */
    [[nodiscard]] reference operator*() const { return reference{n_, *it_}; }

    /** @brief Advance index and base iterator together. */
    enumerate_iterator& operator++()
    {
        ++it_;
        ++n_;
        return *this;
    }

    /** @brief Post-increment. */
    enumerate_iterator operator++(int)
    {
        auto t = *this;
        ++*this;
        return t;
    }

    /** @brief Equality on underlying iterator only. */
    [[nodiscard]] friend bool operator==(enumerate_iterator const& a, enumerate_iterator const& b) noexcept
    {
        return a.it_ == b.it_;
    }
    [[nodiscard]] friend bool operator!=(enumerate_iterator const& a, enumerate_iterator const& b) noexcept
    {
        return !(a == b);
    }
};

/**
 * @ingroup linq
 * @brief Lazy `Select((i, x) => ...)` index overload equivalent.
 */
template <class BaseIt>
class enumerate_view : public query_range_algorithms<enumerate_view<BaseIt>, enumerate_iterator<BaseIt>> {
    BaseIt b_{}, e_{};

public:
    /**
     * @param b Underlying begin (index `0` at `begin()`).
     * @param e Underlying end.
     */
    enumerate_view(BaseIt b, BaseIt e) : b_(std::move(b)), e_(std::move(e)) {}

    /** @brief Index `0` at underlying begin. */
    [[nodiscard]] enumerate_iterator<BaseIt> begin() const { return {b_, 0}; }
    /** @brief End iterator (index unused for equality). */
    [[nodiscard]] enumerate_iterator<BaseIt> end() const { return {e_, 0}; }
};

// ---------------------------------------------------------------------------
// chunk_view — consecutive slices as \c std::vector (lazy fill per chunk).
// ---------------------------------------------------------------------------

/**
 * @ingroup linq
 * @brief Each position holds a `vector` chunk filled on demand (`load_chunk`).
 */
template <class BaseIt>
class chunk_iterator {
    BaseIt cur_{};
    BaseIt end_{};
    std::size_t n_{1};
    mutable std::vector<std::remove_cv_t<typename std::iterator_traits<BaseIt>::value_type>> buf_{};
    bool done_{true};

    void load_chunk()
    {
        buf_.clear();
        if (cur_ == end_) {
            done_ = true;
            return;
        }
        done_ = false;
        buf_.reserve(n_);
        using elem_t = std::remove_cv_t<typename std::iterator_traits<BaseIt>::value_type>;
        for (std::size_t i = 0; i < n_ && cur_ != end_; ++i, ++cur_)
            buf_.push_back(static_cast<elem_t>(*cur_));
    }

public:
    using iterator_category = std::forward_iterator_tag;
    using chunk_vec = std::vector<std::remove_cv_t<typename std::iterator_traits<BaseIt>::value_type>>;
    using value_type = chunk_vec;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = chunk_vec const&;

    chunk_iterator() = default;

    /**
     * @param b Source begin.
     * @param e Source end.
     * @param chunk Max elements per `operator*` (clamped to ≥ 1).
     * @param is_end End iterator if true.
     */
    chunk_iterator(BaseIt b, BaseIt e, std::size_t chunk, bool is_end)
        : cur_(std::move(b)), end_(std::move(e)), n_(chunk < 1 ? 1 : chunk)
    {
        if (is_end) {
            cur_ = end_;
            done_ = true;
            buf_.clear();
        } else {
            load_chunk();
            if (buf_.empty())
                done_ = true;
        }
    }

    /** @brief Current chunk buffer (filled by `load_chunk`). */
    [[nodiscard]] reference operator*() const { return buf_; }

    /** @brief Advance to next chunk or mark done. */
    chunk_iterator& operator++()
    {
        if (done_)
            return *this;
        load_chunk();
        if (buf_.empty())
            done_ = true;
        return *this;
    }

    chunk_iterator operator++(int)
    {
        auto t = *this;
        ++*this;
        return t;
    }

    [[nodiscard]] friend bool operator==(chunk_iterator const& a, chunk_iterator const& b) noexcept
    {
        return a.done_ == b.done_ && a.cur_ == b.cur_;
    }
    [[nodiscard]] friend bool operator!=(chunk_iterator const& a, chunk_iterator const& b) noexcept
    {
        return !(a == b);
    }
};

/**
 * @ingroup linq
 * @brief Splits the sequence into contiguous chunks of size `n` (last chunk may be shorter).
 */
template <class BaseIt>
class chunk_view : public query_range_algorithms<chunk_view<BaseIt>, chunk_iterator<BaseIt>> {
    BaseIt b_{}, e_{};
    std::size_t n_{1};

public:
    /**
     * @param b Source begin.
     * @param e Source end.
     * @param chunk Chunk size (clamped to ≥ 1).
     */
    chunk_view(BaseIt b, BaseIt e, std::size_t chunk) : b_(std::move(b)), e_(std::move(e)), n_(chunk < 1 ? 1 : chunk)
    {}

    [[nodiscard]] chunk_iterator<BaseIt> begin() const { return chunk_iterator<BaseIt>(b_, e_, n_, false); }
    [[nodiscard]] chunk_iterator<BaseIt> end() const { return chunk_iterator<BaseIt>(b_, e_, n_, true); }
};

// ---------------------------------------------------------------------------
// stride_view — every \p step-th element (step clamped to at least 1).
// ---------------------------------------------------------------------------

/**
 * @ingroup linq
 * @brief Advances by `step` positions per increment (clamped to ≥ 1).
 */
template <class BaseIt>
class stride_iterator {
    BaseIt end_{};
    BaseIt cur_{};
    std::size_t step_{1};

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename std::iterator_traits<BaseIt>::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = typename std::iterator_traits<BaseIt>::reference;

    stride_iterator() = default;
    /**
     * @param b Source begin.
     * @param e Source end.
     * @param step Stride (clamped to ≥ 1).
     * @param is_end End sentinel if true.
     */
    stride_iterator(BaseIt b, BaseIt e, std::size_t step, bool is_end)
        : end_(std::move(e)), cur_(is_end ? end_ : std::move(b)), step_(step < 1 ? 1 : step)
    {}

    /** @brief Current element. */
    [[nodiscard]] reference operator*() const { return *cur_; }

    /** @brief Advance by `step` positions (or to end). Random-access bases use O(1) jumps. */
    stride_iterator& operator++()
    {
        using cat = typename std::iterator_traits<BaseIt>::iterator_category;
        if constexpr (std::is_base_of_v<std::random_access_iterator_tag, cat>) {
            if (cur_ == end_)
                return *this;
            using diff_t = typename std::iterator_traits<BaseIt>::difference_type;
            diff_t const rem = end_ - cur_;
            if (rem <= 0)
                return *this;
            std::size_t const take = (std::min)(step_, static_cast<std::size_t>(rem));
            cur_ += static_cast<diff_t>(take);
        } else {
            for (std::size_t i = 0; i < step_ && cur_ != end_; ++i, ++cur_) {}
        }
        return *this;
    }

    /** @brief Post-increment. */
    stride_iterator operator++(int)
    {
        auto t = *this;
        ++*this;
        return t;
    }

    [[nodiscard]] friend bool operator==(stride_iterator const& a, stride_iterator const& b) noexcept
    {
        return a.cur_ == b.cur_;
    }
    [[nodiscard]] friend bool operator!=(stride_iterator const& a, stride_iterator const& b) noexcept
    {
        return !(a == b);
    }
};

/**
 * @ingroup linq
 * @brief Every k-th element of the underlying range.
 */
template <class BaseIt>
class stride_view : public query_range_algorithms<stride_view<BaseIt>, stride_iterator<BaseIt>> {
    BaseIt b_{}, e_{};
    std::size_t step_{1};

public:
    /**
     * @param b Source begin.
     * @param e Source end.
     * @param step Step between yielded elements (clamped to ≥ 1).
     */
    stride_view(BaseIt b, BaseIt e, std::size_t step)
        : b_(std::move(b)), e_(std::move(e)), step_(step < 1 ? 1 : step)
    {}

    [[nodiscard]] stride_iterator<BaseIt> begin() const { return {b_, e_, step_, false}; }
    [[nodiscard]] stride_iterator<BaseIt> end() const { return {b_, e_, step_, true}; }
};

// ---------------------------------------------------------------------------
// scan_view — lazy prefix fold: yields \c f(acc,x) per element (empty → no values).
// ---------------------------------------------------------------------------

/**
 * @brief Lazy `operator*` cache for `scan_iterator` (`bool` + `Acc` when `Acc` is default-constructible).
 */
template <class Acc, bool Direct = std::is_default_constructible_v<Acc>>
struct scan_acc_cache;

template <class Acc>
struct scan_acc_cache<Acc, false> {
    std::optional<Acc> storage_{};
    [[nodiscard]] bool has() const noexcept { return storage_.has_value(); }
    [[nodiscard]] Acc& ref() { return *storage_; }
    [[nodiscard]] Acc const& ref() const { return *storage_; }
    void clear() noexcept { storage_.reset(); }
    template <class T>
    void assign(T&& t)
    {
        storage_.emplace(std::forward<T>(t));
    }
};

template <class Acc>
struct scan_acc_cache<Acc, true> {
    Acc storage_{};
    bool valid_{false};
    [[nodiscard]] bool has() const noexcept { return valid_; }
    [[nodiscard]] Acc& ref() { return storage_; }
    [[nodiscard]] Acc const& ref() const { return storage_; }
    void clear() noexcept { valid_ = false; }
    template <class T>
    void assign(T&& t)
    {
        storage_ = std::forward<T>(t);
        valid_ = true;
    }
};

/**
 * @ingroup linq
 * @brief Caches the next accumulator value; `fn` points at the owning view’s `F` (null at end / inactive).
 * @details End iterator holds `cur == end` and null `fn_`. Callers must not dereference iterators past the
 *          lifetime of the `scan_view` that supplied `fn_`.
 */
template <class BaseIt, class Acc, class F>
class scan_iterator {
    BaseIt cur_{};
    BaseIt end_{};
    Acc acc_{};
    F* fn_{};
    mutable scan_acc_cache<Acc> cache_{};

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Acc;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = Acc const&;

    scan_iterator() = default;

    /**
     * @brief End iterator: `cur == end`, no `F` (only `end()` uses this overload).
     * @param end_only Physical end (past-the-end iterator only).
     */
    explicit scan_iterator(BaseIt end_only) : cur_(std::move(end_only)), end_(cur_) {}

    /**
     * @param b Source begin.
     * @param e Source end.
     * @param seed Initial accumulator (before first element).
     * @param fn Address of the fold function in the parent `scan_view`, or null.
     */
    scan_iterator(BaseIt b, BaseIt e, Acc seed, F* fn)
        : cur_(std::move(b)), end_(std::move(e)), acc_(std::move(seed)), fn_(fn)
    {
        if (cur_ == end_)
            fn_ = nullptr;
    }

    /** @brief Next accumulated value (lazy; cached on first read per position). */
    [[nodiscard]] reference operator*() const
    {
        if (!fn_)
            throw std::out_of_range("qb::linq::scan_iterator::operator*");
        if (!cache_.has())
            cache_.assign((*fn_)(acc_, *cur_));
        return cache_.ref();
    }

    /** @brief Advance: commit accumulator, clear cache, step base. */
    scan_iterator& operator++()
    {
        if (!fn_)
            return *this;
        if (cache_.has())
            acc_ = std::move(cache_.ref());
        else
            acc_ = (*fn_)(acc_, *cur_);
        cache_.clear();
        ++cur_;
        if (cur_ == end_)
            fn_ = nullptr;
        return *this;
    }

    /** @brief Post-increment. */
    scan_iterator operator++(int)
    {
        auto t = *this;
        ++*this;
        return t;
    }

    /** @brief Equality on position and active fold state. */
    [[nodiscard]] friend bool operator==(scan_iterator const& a, scan_iterator const& b) noexcept
    {
        return a.cur_ == b.cur_ && static_cast<bool>(a.fn_) == static_cast<bool>(b.fn_);
    }
    [[nodiscard]] friend bool operator!=(scan_iterator const& a, scan_iterator const& b) noexcept
    {
        return !(a == b);
    }
};

/**
 * @ingroup linq
 * @brief Lazy inclusive scan / running fold (`Scan` / prefix of `Aggregate`).
 */
template <class BaseIt, class Acc, class F>
class scan_view : public query_range_algorithms<scan_view<BaseIt, Acc, F>, scan_iterator<BaseIt, Acc, F>> {
    BaseIt b_{}, e_{};
    Acc seed_{};
    /** @brief `mutable` so `begin() const` can expose `F*` for non-const callables under the same rules as
     *         `shared_ptr<F>::get()` in a const member function. */
    mutable F fn_{};

public:
    /**
     * @param b Source begin.
     * @param e Source end.
     * @param s Seed accumulator.
     * @param fn Fold function (stored in the view).
     */
    scan_view(BaseIt b, BaseIt e, Acc s, F fn)
        : b_(std::move(b)), e_(std::move(e)), seed_(std::move(s)), fn_(std::move(fn))
    {}

    /** @brief Iterator at `b` with seed; points at this view’s `fn_` (null when `[b,e)` empty). */
    [[nodiscard]] scan_iterator<BaseIt, Acc, F> begin() const
    {
        F* p = (b_ == e_) ? nullptr : std::addressof(fn_);
        return scan_iterator<BaseIt, Acc, F>(b_, e_, seed_, p);
    }
    /** @brief End (no yields when `[b,e)` empty). */
    [[nodiscard]] scan_iterator<BaseIt, Acc, F> end() const
    {
        return scan_iterator<BaseIt, Acc, F>(e_);
    }
};

// ---------------------------------------------------------------------------
// query_range_algorithms — concat / zip / default_if_empty / distinct
// (definitions after view types; declarations in query_range.h)
// ---------------------------------------------------------------------------

/** @brief Out-of-line `enumerate`: `(index, element)` over `derived()` range. */
template <class Derived, class Iter>
enumerate_view<typename query_range_algorithms<Derived, Iter>::iterator>
query_range_algorithms<Derived, Iter>::enumerate() const
{
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    return enumerate_view<iterator_t>(derived().begin(), derived().end());
}

/** @brief Out-of-line `scan`: prefix fold; `F` lives in the returned `scan_view`. */
template <class Derived, class Iter>
template <class Acc, class F>
scan_view<Iter, std::decay_t<Acc>, std::decay_t<F>> query_range_algorithms<Derived, Iter>::scan(
    Acc&& seed, F&& f) const
{
    using acc_t = std::decay_t<Acc>;
    using f_t = std::decay_t<F>;
    return scan_view<Iter, acc_t, f_t>(
        derived().begin(), derived().end(), static_cast<acc_t>(std::forward<Acc>(seed)), static_cast<f_t>(std::forward<F>(f)));
}

/** @brief Out-of-line `concat`: this range then `rhs` (same `value_type`). */
template <class Derived, class Iter>
template <class Rng>
concat_view<typename query_range_algorithms<Derived, Iter>::iterator,
    decltype(std::declval<Rng const&>().begin())>
query_range_algorithms<Derived, Iter>::concat(Rng const& rhs) const
{
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    static_assert(std::is_same_v<iter_value_t<iterator_t>, iter_value_t<decltype(rhs.begin())>>,
        "qb::linq::concat requires the same element type on both sides");
    return concat_view<iterator_t, decltype(rhs.begin())>(
        derived().begin(), derived().end(), rhs.begin(), rhs.end());
}

/** @brief Out-of-line `zip`: pairwise until shorter range ends. */
template <class Derived, class Iter>
template <class Rng>
zip_view<typename query_range_algorithms<Derived, Iter>::iterator,
    decltype(std::declval<Rng const&>().begin())>
query_range_algorithms<Derived, Iter>::zip(Rng const& rhs) const
{
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    return zip_view<iterator_t, decltype(rhs.begin())>(
        derived().begin(), derived().end(), rhs.begin(), rhs.end());
}

/** @brief Out-of-line `zip` (three ranges): shortest length wins. */
template <class Derived, class Iter>
template <class Rng2, class Rng3>
zip3_view<typename query_range_algorithms<Derived, Iter>::iterator,
    decltype(std::declval<Rng2 const&>().begin()), decltype(std::declval<Rng3 const&>().begin())>
query_range_algorithms<Derived, Iter>::zip(Rng2 const& r2, Rng3 const& r3) const
{
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    return zip3_view<iterator_t, decltype(r2.begin()), decltype(r3.begin())>(derived().begin(), derived().end(),
        r2.begin(), r2.end(), r3.begin(), r3.end());
}

/** @brief Out-of-line `default_if_empty`: one `def` when `derived()` is empty. */
template <class Derived, class Iter>
auto query_range_algorithms<Derived, Iter>::default_if_empty(value_type def) const
    -> default_if_empty_view<Iter, value_type>
{
    return default_if_empty_view<Iter, value_type>(derived().begin(), derived().end(), std::move(def));
}

/** @brief Out-of-line `distinct_by`: first occurrence per `kf(*it)` key. */
template <class Derived, class Iter>
template <class KF>
auto query_range_algorithms<Derived, Iter>::distinct_by(KF&& kf) const
    -> distinct_view<Iter, std::decay_t<KF>,
        std::decay_t<std::invoke_result_t<KF&, reference>>>
{
    using key_t = std::decay_t<std::invoke_result_t<KF&, reference>>;
    return distinct_view<Iter, std::decay_t<KF>, key_t>(
        derived().begin(), derived().end(), std::forward<KF>(kf));
}

/** @brief Out-of-line `distinct`: identity key (`identity_proj`). */
template <class Derived, class Iter>
auto query_range_algorithms<Derived, Iter>::distinct() const
    -> distinct_view<Iter, identity_proj<reference>, value_type>
{
    return distinct_by(identity_proj<reference>{});
}

/** @brief Out-of-line `union_with`: `concat(rhs).distinct()`. */
template <class Derived, class Iter>
template <class Rng>
auto query_range_algorithms<Derived, Iter>::union_with(Rng const& rhs) const
{
    return concat(rhs).distinct();
}

/** @brief Out-of-line `chunk`: vectors of up to `size` elements. */
template <class Derived, class Iter>
chunk_view<Iter> query_range_algorithms<Derived, Iter>::chunk(std::size_t size) const
{
    return chunk_view<Iter>(derived().begin(), derived().end(), size);
}

/** @brief Out-of-line `stride`: every `step`-th element. */
template <class Derived, class Iter>
stride_view<Iter> query_range_algorithms<Derived, Iter>::stride(std::size_t step) const
{
    return stride_view<Iter>(derived().begin(), derived().end(), step);
}

/** @brief Out-of-line `append`: `concat(single_view(v))`. */
template <class Derived, class Iter>
template <class T>
auto query_range_algorithms<Derived, Iter>::append(T&& v) const
{
    using U = std::decay_t<T>;
    return concat(single_view<U>(std::forward<T>(v)));
}

/** @brief Out-of-line `prepend`: `single_view(v).concat(derived())`. */
template <class Derived, class Iter>
template <class T>
auto query_range_algorithms<Derived, Iter>::prepend(T&& v) const
{
    using U = std::decay_t<T>;
    return single_view<U>(std::forward<T>(v)).concat(derived());
}

} // namespace detail
} // namespace qb::linq

// ---- qb/linq/query.h ----

/**
 * @file query.h
 * @ingroup linq
 * @brief Core range types: `query_state`, lazy views (`select`/`where`/`take`), `iota_view`, `materialized_range`.
 *
 * @details
 * Includes `detail/extra_views.h` at the end of the dependency chain so `query_range_algorithms` out-of-line
 * definitions can instantiate `concat_view`, `zip_view`, etc. User code should include `qb/linq.h` rather than
 * this header directly.
 *
 * @par Dependency order
 * `query_range.h` (declarations) → `iterators.h` → `reverse_iterator.h` → `extra_views.h` (definitions) → template bodies below for
 * `group_by`, `order_by`, `to_*`, `join`, set ops (require complete `materialized_range`).
 *
 * @par Types in this header
 * | Type | Role |
 * |------|------|
 * | `query_state` | Minimal `[begin,end)` + CRTP algorithms |
 * | `reversed_view` | `qb::linq::reverse_iterator` adapter (no default ctor on adaptor) |
 * | `subrange` | Slice after `skip` / `skip_while` |
 * | `from_range` | `basic_iterator` wrapper for raw iterators |
 * | `select_view` / `where_view` / `take_n_view` / `take_while_view` | Lazy views |
 * | `iota_iterator` / `iota_view` | Arithmetic sequence (no container) |
 * | `materialized_range` | `shared_ptr` to owner + iterators |
 */



namespace qb::linq {

namespace detail {

/**
 * @ingroup linq
 * @brief SFINAE: `Owner` has `at(K const&)` (used for `materialized_range::operator[]`).
 * @tparam Owner Container or map type held by `shared_ptr` in `materialized_range`.
 * @tparam K Key type passed to `operator[]`.
 */
template <class Owner, class K, class = void>
struct has_at : std::false_type {};

/**
 * @ingroup linq
 * @brief `std::true_type` when `Owner::at(K const&)` is valid.
 */
template <class Owner, class K>
struct has_at<Owner, K, std::void_t<decltype(std::declval<Owner&>().at(std::declval<K const&>()))>>
    : std::true_type {};

} // namespace detail

// ---------------------------------------------------------------------------
// query_state — stores iterator pair; algorithms from query_range_algorithms
// ---------------------------------------------------------------------------

/**
 * @ingroup linq
 * @brief Minimal handle: stores `[begin, end)` and inherits all `query_range_algorithms`.
 * @tparam Iter Iterator type for the sequence.
 */
template <class Iter>
class query_state : public detail::query_range_algorithms<query_state<Iter>, Iter> {
public:
    using iterator = Iter;
    using reference = detail::iter_reference_t<Iter>;
    using value_type = std::remove_const_t<std::remove_reference_t<reference>>;

protected:
    Iter begin_{};
    Iter end_{};

public:
    /** @name Construction (query_state) */
    /** @{ */
    /** @brief Default construction (null range; iterators may be value-initialized — use with care). */
    query_state() = default;
    /**
     * @param b Half-open start iterator.
     * @param e Half-open end iterator (non-owning).
     */
    query_state(Iter b, Iter e) : begin_(std::move(b)), end_(std::move(e)) {}
    /** @} */

    /** @name Iteration */
    /** @{ */
    /** @brief Start of the half-open range. */
    [[nodiscard]] Iter begin() const { return begin_; }
    /** @brief Past-the-end iterator. */
    [[nodiscard]] Iter end() const { return end_; }
    /** @} */
};

/**
 * @ingroup linq
 * @brief Reverse iteration over `[src.begin(), src.end())` using `qb::linq::reverse_iterator`.
 * @tparam Iter Underlying iterator type from the adapted range (at least bidirectional).
 */
template <class Iter>
class reversed_view : public query_state<reverse_iterator<Iter>> {
    using base_t = query_state<reverse_iterator<Iter>>;

public:
    /**
     * @brief Adapts `src` so iteration runs from `src.end()` toward `src.begin()` (bidirectional requirement on `Rng`).
     * @tparam Rng Any type with `begin`/`end` yielding iterators convertible from `Iter`’s source category.
     * @param src Range to traverse in reverse order.
     */
    template <class Rng, class = std::void_t<decltype(std::declval<Rng>().begin())>>
    explicit reversed_view(Rng const& src)
        : base_t(reverse_iterator<Iter>(src.end()), reverse_iterator<Iter>(src.begin()))
    {}
};

/**
 * @ingroup linq
 * @brief `take_n_iterator` uses `(physical_end, 0)` as a logical end sentinel (`operator==` matches any `(base, 0)`).
 *        Generic `reversed_view` would wrap that physical end and walk the whole underlying range; scan once to the
 *        exhausted position. An exhausted iterator still sits **on** the last element (`remaining_ == 0`); `reverse_iterator`
 *        needs a position one **past** that element on the underlying base, so only `BaseIt` is bumped (`remaining_`
 *        unchanged).
 */
template <class BaseIt>
class reversed_view<take_n_iterator<BaseIt>> : public query_state<reverse_iterator<take_n_iterator<BaseIt>>> {
    using It = take_n_iterator<BaseIt>;
    using base_t = query_state<reverse_iterator<It>>;

    static It scan_logical_end(It b, It const& sen)
    {
        It cur = std::move(b);
        while (cur != sen)
            ++cur;
        return cur;
    }

    static It bump_base_past_last(It exh)
    {
        It r = std::move(exh);
        ++static_cast<BaseIt&>(r);
        return r;
    }

    template <class Rng>
    static base_t make_bounds(Rng const& src)
    {
        It b = src.begin();
        It const sen = src.end();
        It exh = scan_logical_end(It(b), sen);
        if (b == exh) {
            reverse_iterator<It> const r(b);
            return base_t(r, r);
        }
        if (static_cast<BaseIt const&>(exh) == static_cast<BaseIt const&>(sen))
            return base_t(reverse_iterator<It>(std::move(exh)), reverse_iterator<It>(std::move(b)));
        return base_t(reverse_iterator<It>(bump_base_past_last(std::move(exh))), reverse_iterator<It>(std::move(b)));
    }

public:
    template <class Rng, class = std::void_t<decltype(std::declval<Rng>().begin())>>
    explicit reversed_view(Rng const& src)
        : base_t(make_bounds(src))
    {}
};

/**
 * @ingroup linq
 * @brief After a forward scan, the logical end iterator sits on the **first failing** element; its underlying base is a
 *        valid half-open end for the accepted prefix (unlike `take_n_iterator`, which stops **on** the last taken
 *        element). Reverse using `reverse_iterator<BaseIt>` only: nesting `take_while_iterator` inside `reverse_iterator`
 *        breaks equality (asymmetric `operator==`; predicate on the left can equate unrelated positions).
 */
template <class BaseIt, class Pred>
class reversed_view<take_while_iterator<BaseIt, Pred>> : public query_state<reverse_iterator<BaseIt>> {
    using Tw = take_while_iterator<BaseIt, Pred>;
    using base_t = query_state<reverse_iterator<BaseIt>>;

    template <class Rng>
    static base_t make_bounds(Rng const& src)
    {
        Tw b = src.begin();
        Tw const sen = src.end();
        Tw cur = b;
        bool any_step = false;
        while (cur != sen) {
            any_step = true;
            ++cur;
        }
        Tw exh = std::move(cur);
        BaseIt const bb = static_cast<BaseIt const&>(b);
        BaseIt const ee = static_cast<BaseIt const&>(exh);
        if (!any_step) {
            reverse_iterator<BaseIt> const r(bb);
            return base_t(r, r);
        }
        return base_t(reverse_iterator<BaseIt>(ee), reverse_iterator<BaseIt>(bb));
    }

public:
    template <class Rng, class = std::void_t<decltype(std::declval<Rng>().begin())>>
    explicit reversed_view(Rng const& src)
        : base_t(make_bounds(src))
    {}
};

/**
 * @ingroup linq
 * @brief Half-open subrange `[b, e)` without adapting iterators (used by `skip` / `skip_while`).
 */
template <class Iter>
class subrange : public query_state<Iter> {
public:
    /**
     * @brief Subrange adapter; iterators must remain valid for the source’s lifetime.
     * @param b New range begin (after `skip` / `skip_while`).
     * @param e Original end.
     */
    subrange(Iter b, Iter e) : query_state<Iter>(std::move(b), std::move(e)) {}
};

/**
 * @ingroup linq
 * @brief Adapts raw iterators to `basic_iterator` for uniform pipeline typing (`from` / `as_enumerable`).
 */
template <class RawIter>
class from_range : public query_state<basic_iterator<RawIter>> {
public:
    /**
     * @brief Wraps raw iterators in `basic_iterator` for CRTP / pipeline uniformity.
     * @param b Half-open begin.
     * @param e Half-open end (non-owning).
     */
    from_range(RawIter b, RawIter e)
        : query_state<basic_iterator<RawIter>>(basic_iterator<RawIter>(std::move(b)),
              basic_iterator<RawIter>(std::move(e)))
    {}
};

/**
 * @ingroup linq
 * @brief Lazy `select`: iterator pair of `select_iterator<BaseIt, F>`.
 */
template <class BaseIt, class F>
class select_view : public query_state<select_iterator<BaseIt, F>> {
public:
    /**
     * @brief Lazy projection over `[b,e)`; `f` is copied into both endpoint iterators.
     * @param b Source begin.
     * @param e Source end.
     * @param f Projection; stored in both iterators.
     */
    select_view(BaseIt b, BaseIt e, F f)
        : query_state<select_iterator<BaseIt, F>>(
              select_iterator<BaseIt, F>(std::move(b), f), select_iterator<BaseIt, F>(std::move(e), f))
    {}
};

/**
 * @ingroup linq
 * @brief Lazy filter: O(1) construction; `std::find_if` runs at most once on first `begin()` (cached).
 * @tparam BaseIt Underlying iterator.
 * @tparam P Predicate type (stored in iterators).
 * @details The first-match cache uses `detail::lazy_copy_once<BaseIt>`: no heap allocation and no requirement
 *          that `BaseIt` be default-constructible (unlike `std::optional<BaseIt>` with some `reverse_iterator`
 *          specializations on libstdc++).
 */
template <class BaseIt, class P>
class where_view : public detail::query_range_algorithms<where_view<BaseIt, P>, where_iterator<BaseIt, P>> {
public:
    /** @brief Filter iterator type (`where_iterator`). */
    using iterator = where_iterator<BaseIt, P>;
    using reference = detail::iter_reference_t<iterator>;
    using value_type = std::remove_const_t<std::remove_reference_t<reference>>;

private:
    BaseIt b_{};
    BaseIt e_{};
    P pred_{};
    mutable detail::lazy_copy_once<BaseIt> first_{};

public:
    /** @name Construction (where_view) */
    /** @{ */
    /**
     * @brief Stores bounds and predicate only; first `begin()` may run `find_if` once and cache (`first_`).
     * @note Not thread-safe; repeated `begin()` returns iterators sharing cached first match.
     */
    where_view(BaseIt b, BaseIt e, P pred)
        : b_(std::move(b)), e_(std::move(e)), pred_(std::move(pred))
    {}
    /** @} */

    /** @name Iteration */
    /** @{ */
    /** @brief First element matching `pred_` or `e_`; runs `find_if` at most once (cached). */
    [[nodiscard]] iterator begin() const
    {
        BaseIt const& first_it = first_.cref_or_emplace([&] { return std::find_if(b_, e_, pred_); });
        return iterator(first_it, b_, e_, pred_);
    }

    /** @brief Physical end; do not dereference. */
    [[nodiscard]] iterator end() const { return iterator(e_, b_, e_, pred_); }
    /** @} */
};

/**
 * @ingroup linq
 * @brief Takes the first `n` elements (after normalizing sign / overflow, see `take_n_count`).
 */
template <class BaseIt>
class take_n_view : public query_state<take_n_iterator<BaseIt>> {
public:
    /**
     * @brief Prefix of `[b,e)` of length `take_n_count(n)` (see `take_n_count` for sign / overflow).
     * @param b Underlying sequence begin.
     * @param e Underlying sequence end.
     * @param n Take count (`take_n_count` normalizes sign / overflow; see class brief).
     */
    take_n_view(BaseIt b, BaseIt e, std::ptrdiff_t n)
        : query_state<take_n_iterator<BaseIt>>(
              take_n_iterator<BaseIt>(std::move(b), -take_n_count(n)),
              take_n_iterator<BaseIt>(std::move(e), 0))
    {}

private:
    /**
     * @brief Magnitude of `n` for take count (`n < 0` → `abs`); overflow of negation → `0`.
     * @see take_n_iterator::remaining_
     */
    static std::ptrdiff_t take_n_count(std::ptrdiff_t n) noexcept
    {
        // Guard against negating PTRDIFF_MIN (UB)
        if (n == std::numeric_limits<std::ptrdiff_t>::min())
            return 0;
        return n < 0 ? -n : n;
    }
};

/**
 * @ingroup linq
 * @brief Yields elements while `P` holds; end is logical (see `take_while_iterator`).
 */
template <class BaseIt, class P>
class take_while_view : public query_state<take_while_iterator<BaseIt, P>> {
public:
    /**
     * @brief Prefix of `[b,e)` while `p(*it)`; both iterators share `p` (do not compare across different views).
     */
    take_while_view(BaseIt b, BaseIt e, P p)
        : query_state<take_while_iterator<BaseIt, P>>(
              take_while_iterator<BaseIt, P>(std::move(b), p), take_while_iterator<BaseIt, P>(std::move(e), p))
    {}
};

/**
 * @ingroup linq
 * @brief Forward iterator over a counter value (`T` must be incrementable and equality-comparable).
 * @tparam T Counter / endpoint type stored by value in `iota_view`.
 */
template <class T>
class iota_iterator {
    T n_{};

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::remove_cv_t<T>;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;

    /** @brief Value-initialized counter (typically paired with `end` only in valid ranges). */
    iota_iterator() = default;
    /** @param n Current value (`begin` / `end` of `iota_view` differ by this state only). */
    explicit iota_iterator(T n) noexcept(std::is_nothrow_move_constructible_v<T>) : n_(std::move(n)) {}

    /** @brief Current counter value (by value). */
    [[nodiscard]] reference operator*() const noexcept { return n_; }

    /** @brief Pre-increment (`++n`). */
    iota_iterator& operator++() noexcept(noexcept(++std::declval<T&>()))
    {
        ++n_;
        return *this;
    }

    /** @brief Post-increment. */
    iota_iterator operator++(int) noexcept(noexcept(++std::declval<T&>()))
    {
        auto t = *this;
        ++*this;
        return t;
    }

    /** @brief Equality on counter value. */
    [[nodiscard]] friend bool operator==(iota_iterator const& a, iota_iterator const& b) noexcept
    {
        return a.n_ == b.n_;
    }
    /** @brief Inequality. */
    [[nodiscard]] friend bool operator!=(iota_iterator const& a, iota_iterator const& b) noexcept
    {
        return !(a == b);
    }
};

/**
 * @ingroup linq
 * @brief Lazy half-open `[first, last)` with no backing container (LINQ `Enumerable.Range` style).
 */
template <class T>
class iota_view : public detail::query_range_algorithms<iota_view<T>, iota_iterator<T>> {
    T first_{};
    T last_{};

public:
    /**
     * @brief Half-open counter range `[first, last)` (no storage; `T` must support `++` and equality).
     * @param first Value returned by `begin()`.
     * @param last One-past / end bound (exclusive); empty range if `first == last` in the sense of iterator equality.
     */
    iota_view(T first, T last) noexcept(std::is_nothrow_move_constructible_v<T>)
        : first_(std::move(first)), last_(std::move(last))
    {}

    /** @name Iteration (iota_view) */
    /** @{ */
    /** @brief Iterator at `first` (exclusive end is `last`). */
    [[nodiscard]] iota_iterator<T> begin() const noexcept(
        std::is_nothrow_copy_constructible_v<T> || std::is_nothrow_move_constructible_v<T>)
    {
        return iota_iterator<T>(first_);
    }

    /** @brief Sentinel iterator at `last` (half-open `[first,last)`). */
    [[nodiscard]] iota_iterator<T> end() const noexcept(
        std::is_nothrow_copy_constructible_v<T> || std::is_nothrow_move_constructible_v<T>)
    {
        return iota_iterator<T>(last_);
    }
    /** @} */
};

/**
 * @ingroup linq
 * @brief Owns materialized data via `shared_ptr<Owner>`; exposes iterators and optional `operator[]` / `desc`.
 * @tparam Iter Iterator into `Owner` (e.g. `vector::iterator`, map iterator).
 * @tparam Owner Container or map type kept alive for the lifetime of iterators.
 */
template <class Iter, class Owner>
class materialized_range : public query_state<Iter> {
public:
    /** @name Construction (materialized_range) */
    /** @{ */
    /**
     * @brief Full `[begin(), end())` over `*own`; transfers ownership (preferred when the range is the whole container).
     * @details Base-class iterators are taken from `own` before the `shared_ptr` member is move-assigned, so this
     *          path is safe on all compilers. Use this (or `detail::wrap_materialized`) instead of calling the
     *          three-argument constructor with `owner->begin()`, `owner->end()`, and `std::move(owner)` in one
     *          expression — that pattern is undefined behavior because parameter initialization order is
     *          unspecified (GCC/Linux vs Clang/macOS).
     */
    template <class O = Owner, class = std::enable_if_t<std::is_same_v<Iter, typename O::iterator>>>
    explicit materialized_range(std::shared_ptr<O> own)
        : query_state<Iter>(own->begin(), own->end()), owner_(std::move(own))
    {}

    /**
     * @brief Iterator range over data owned by `own` (shared lifetime).
     * @param b Iterator to first element in `*own` (or map begin).
     * @param e Iterator past the exposed range.
     * @param own Shared ownership; must outlive any iterator derived from this range.
     * @warning Do not pass `std::move(owner)` together with `owner->begin()` / `owner->end()` as sibling arguments
     *          from the same local `owner` — use `materialized_range(std::move(owner))` or `wrap_materialized`
     *          instead. Safe uses: iterators and `own` come from separate prior statements (`desc()`), or `own` is
     *          an lvalue (e.g. a data member) so the third parameter is a copy.
     */
    materialized_range(Iter b, Iter e, std::shared_ptr<Owner> own)
        : query_state<Iter>(std::move(b), std::move(e)), owner_(std::move(own))
    {}
    /** @} */

    /** @name Views and keyed access */
    /** @{ */
    /** @brief Identity view (ascending iteration over stored order). */
    [[nodiscard]] materialized_range asc() const { return *this; }

    /** @brief Reverse iteration over the owner via `rbegin`/`rend` (when available). */
    [[nodiscard]] auto desc() const
    {
        auto rb = owner_->rbegin();
        auto re = owner_->rend();
        using rev_iter = decltype(rb);
        return materialized_range<rev_iter, Owner>(rb, re, owner_);
    }

    /** @brief Random access by key when `Owner` provides `at` (e.g. dictionary / grouped map). */
    template <class K, std::enable_if_t<detail::has_at<Owner, K>::value, int> = 0>
    [[nodiscard]] decltype(auto) operator[](K const& key) const
    {
        return owner_->at(key);
    }

    /** @brief `reverse_iterator` view over the same stored `[begin,end)` span. */
    [[nodiscard]] materialized_range<reverse_iterator<Iter>, Owner> reverse() const
    {
        return materialized_range<reverse_iterator<Iter>, Owner>(
            reverse_iterator<Iter>(this->end_), reverse_iterator<Iter>(this->begin_), owner_);
    }
    /** @} */

private:
    std::shared_ptr<Owner> owner_;
};

// ---------------------------------------------------------------------------
// query_range_algorithms — out-of-line materializing ops (complete materialized_range)
// ---------------------------------------------------------------------------
/**
 * @namespace qb::linq::detail
 * @ingroup linq
 * @brief Template definitions below: `group_by`, `order_by`, container materializers, `join` / `left_join` /
 *        `right_join` / `group_join`, set-like ops. Declarations live in `query_range.h`.
 */
namespace detail {

/**
 * @brief Builds a `materialized_range` over the full contents of a filled `shared_ptr` owner.
 * @details Delegates to the single-argument `materialized_range` constructor (safe base/member init order).
 *          Never replace this with `materialized_range(owner->begin(), owner->end(), std::move(owner))` in a single
 *          call — that is undefined behavior when the three arguments involve the same moved-from `owner`.
 */
template <class Owner>
materialized_range<typename Owner::iterator, Owner> wrap_materialized(std::shared_ptr<Owner> owner)
{
    return materialized_range<typename Owner::iterator, Owner>(std::move(owner));
}

/** @brief Out-of-line `group_by`: populate nested map, return iterators over buckets. */
template <class Derived, class Iter>
template <class... Keys>
materialized_range<typename group_by_map_t<typename std::iterator_traits<Iter>::reference, Keys...>::iterator,
    group_by_map_t<typename std::iterator_traits<Iter>::reference, Keys...>>
query_range_algorithms<Derived, Iter>::group_by(Keys&&... keys) const
{
    using ref = typename std::iterator_traits<Iter>::reference;
    using map_t = group_by_map_t<ref, Keys...>;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    auto owner = std::make_shared<map_t>();
    for (iterator_t it = derived().begin(); it != derived().end(); ++it)
        group_by_impl<ref, std::decay_t<Keys>...>::emplace(*owner, *it, keys...);
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `order_by`: copy to vector, `std::stable_sort` with lexicographic key extractors. */
template <class Derived, class Iter>
template <class... KeyFilters>
materialized_range<typename std::vector<typename query_range_algorithms<Derived, Iter>::value_type>::iterator,
    std::vector<typename query_range_algorithms<Derived, Iter>::value_type>>
query_range_algorithms<Derived, Iter>::order_by(KeyFilters&&... key_filters) const
{
    using val_t = typename query_range_algorithms<Derived, Iter>::value_type;
    auto owner = std::make_shared<std::vector<val_t>>();
    reserve_if_random_access(*owner, derived().begin(), derived().end());
    std::copy(derived().begin(), derived().end(), std::back_inserter(*owner));
    // Use `val_t const&`, not `reference`: `std::stable_sort` (MSVC in particular) may call the predicate with
    // `const` arguments; `lexicographic_compare` already takes `In const&`.
    std::stable_sort(owner->begin(), owner->end(), [&](val_t const& a, val_t const& b) {
        return lexicographic_compare(a, b, key_filters...);
    });
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `to_vector`: copy sequence into `std::vector` owned by `materialized_range`. */
template <class Derived, class Iter>
materialized_range<typename std::vector<typename query_range_algorithms<Derived, Iter>::value_type>::iterator,
    std::vector<typename query_range_algorithms<Derived, Iter>::value_type>>
query_range_algorithms<Derived, Iter>::to_vector() const
{
    using val_t = typename query_range_algorithms<Derived, Iter>::value_type;
    auto owner = std::make_shared<std::vector<val_t>>();
    reserve_if_random_access(*owner, derived().begin(), derived().end());
    std::copy(derived().begin(), derived().end(), std::back_inserter(*owner));
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `to_unordered_map`: hash map; last write wins on duplicate keys. */
template <class Derived, class Iter>
template <class KeyFn, class ElemFn>
auto query_range_algorithms<Derived, Iter>::to_unordered_map(KeyFn&& keyf, ElemFn&& elemf) const
{
    using ref = typename query_range_algorithms<Derived, Iter>::reference;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    using K = std::decay_t<std::invoke_result_t<KeyFn&, ref>>;
    using V = std::decay_t<std::invoke_result_t<ElemFn&, ref>>;
    using map_t = std::unordered_map<K, V>;
    auto owner = std::make_shared<map_t>();
    reserve_if_random_access(*owner, derived().begin(), derived().end());
    for (iterator_t it = derived().begin(); it != derived().end(); ++it)
        (*owner)[keyf(*it)] = elemf(*it);
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `to_map`: ordered `std::map`; last write wins on duplicate keys. */
template <class Derived, class Iter>
template <class KeyFn, class ElemFn>
auto query_range_algorithms<Derived, Iter>::to_map(KeyFn&& keyf, ElemFn&& elemf) const
{
    using ref = typename query_range_algorithms<Derived, Iter>::reference;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    using K = std::decay_t<std::invoke_result_t<KeyFn&, ref>>;
    using V = std::decay_t<std::invoke_result_t<ElemFn&, ref>>;
    using map_t = std::map<K, V>;
    auto owner = std::make_shared<map_t>();
    for (iterator_t it = derived().begin(); it != derived().end(); ++it)
        (*owner)[keyf(*it)] = elemf(*it);
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `to_dictionary`: `unordered_map` with duplicate-key detection (`invalid_argument`). */
template <class Derived, class Iter>
template <class KeyFn, class ElemFn>
auto query_range_algorithms<Derived, Iter>::to_dictionary(KeyFn&& keyf, ElemFn&& elemf) const
{
    using ref = typename query_range_algorithms<Derived, Iter>::reference;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    using K = std::decay_t<std::invoke_result_t<KeyFn&, ref>>;
    using V = std::decay_t<std::invoke_result_t<ElemFn&, ref>>;
    using map_t = std::unordered_map<K, V>;
    auto owner = std::make_shared<map_t>();
    reserve_if_random_access(*owner, derived().begin(), derived().end());
    for (iterator_t it = derived().begin(); it != derived().end(); ++it) {
        K const k = keyf(*it);
        if (!owner->emplace(k, elemf(*it)).second)
            throw std::invalid_argument("qb::linq::to_dictionary: duplicate key");
    }
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `except`: set difference vs `rhs` (C# `Enumerable.Except`); distinct left values, first-seen order. */
template <class Derived, class Iter>
template <class Rng>
materialized_range<typename std::vector<typename query_range_algorithms<Derived, Iter>::value_type>::iterator,
    std::vector<typename query_range_algorithms<Derived, Iter>::value_type>>
query_range_algorithms<Derived, Iter>::except(Rng const& rhs) const
{
    using val_t = typename query_range_algorithms<Derived, Iter>::value_type;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    std::unordered_set<val_t> ban;
    reserve_if_random_access(ban, rhs.begin(), rhs.end());
    for (auto it = rhs.begin(); it != rhs.end(); ++it)
        ban.insert(*it);
    std::unordered_set<val_t> yielded;
    auto owner = std::make_shared<std::vector<val_t>>();
    reserve_if_random_access(*owner, derived().begin(), derived().end());
    for (iterator_t it = derived().begin(); it != derived().end(); ++it) {
        val_t const& v = *it;
        if (ban.find(v) == ban.end() && yielded.insert(v).second)
            owner->push_back(v);
    }
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `intersect`: set intersection (C# `Enumerable.Intersect`); distinct left values, first-seen order. */
template <class Derived, class Iter>
template <class Rng>
materialized_range<typename std::vector<typename query_range_algorithms<Derived, Iter>::value_type>::iterator,
    std::vector<typename query_range_algorithms<Derived, Iter>::value_type>>
query_range_algorithms<Derived, Iter>::intersect(Rng const& rhs) const
{
    using val_t = typename query_range_algorithms<Derived, Iter>::value_type;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    std::unordered_set<val_t> want;
    reserve_if_random_access(want, rhs.begin(), rhs.end());
    for (auto it = rhs.begin(); it != rhs.end(); ++it)
        want.insert(*it);
    std::unordered_set<val_t> yielded;
    auto owner = std::make_shared<std::vector<val_t>>();
    reserve_if_random_access(*owner, derived().begin(), derived().end());
    for (iterator_t it = derived().begin(); it != derived().end(); ++it) {
        val_t const& v = *it;
        if (want.find(v) != want.end() && yielded.insert(v).second)
            owner->push_back(v);
    }
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `except_by`: distinct left elements by key, excluding keys present on the right. */
template <class Derived, class Iter>
template <class Rng, class KeyLeft, class KeyRight>
materialized_range<typename std::vector<typename query_range_algorithms<Derived, Iter>::value_type>::iterator,
    std::vector<typename query_range_algorithms<Derived, Iter>::value_type>>
query_range_algorithms<Derived, Iter>::except_by(
    Rng const& rhs, KeyLeft&& key_left, KeyRight&& key_right) const
{
    using val_t = typename query_range_algorithms<Derived, Iter>::value_type;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    using ref_l = typename query_range_algorithms<Derived, Iter>::reference;
    using rri = decltype(rhs.begin());
    using ref_r = decltype(*std::declval<rri>());
    using K = std::decay_t<std::invoke_result_t<KeyLeft&, ref_l>>;
    static_assert(std::is_same_v<K, std::decay_t<std::invoke_result_t<KeyRight&, ref_r>>>,
        "qb::linq::except_by: key types from left and right selectors must match");
    std::unordered_set<K> ban;
    reserve_if_random_access(ban, rhs.begin(), rhs.end());
    for (auto it = rhs.begin(); it != rhs.end(); ++it)
        ban.insert(key_right(*it));
    std::unordered_set<K> yielded_keys;
    auto owner = std::make_shared<std::vector<val_t>>();
    reserve_if_random_access(*owner, derived().begin(), derived().end());
    for (iterator_t it = derived().begin(); it != derived().end(); ++it) {
        K const k = key_left(*it);
        if (ban.find(k) == ban.end() && yielded_keys.insert(k).second)
            owner->push_back(*it);
    }
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `intersect_by`: distinct left elements by key whose key appears on the right. */
template <class Derived, class Iter>
template <class Rng, class KeyLeft, class KeyRight>
materialized_range<typename std::vector<typename query_range_algorithms<Derived, Iter>::value_type>::iterator,
    std::vector<typename query_range_algorithms<Derived, Iter>::value_type>>
query_range_algorithms<Derived, Iter>::intersect_by(
    Rng const& rhs, KeyLeft&& key_left, KeyRight&& key_right) const
{
    using val_t = typename query_range_algorithms<Derived, Iter>::value_type;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    using ref_l = typename query_range_algorithms<Derived, Iter>::reference;
    using rri = decltype(rhs.begin());
    using ref_r = decltype(*std::declval<rri>());
    using K = std::decay_t<std::invoke_result_t<KeyLeft&, ref_l>>;
    static_assert(std::is_same_v<K, std::decay_t<std::invoke_result_t<KeyRight&, ref_r>>>,
        "qb::linq::intersect_by: key types from left and right selectors must match");
    std::unordered_set<K> want;
    reserve_if_random_access(want, rhs.begin(), rhs.end());
    for (auto it = rhs.begin(); it != rhs.end(); ++it)
        want.insert(key_right(*it));
    std::unordered_set<K> yielded_keys;
    auto owner = std::make_shared<std::vector<val_t>>();
    reserve_if_random_access(*owner, derived().begin(), derived().end());
    for (iterator_t it = derived().begin(); it != derived().end(); ++it) {
        K const k = key_left(*it);
        if (want.find(k) != want.end() && yielded_keys.insert(k).second)
            owner->push_back(*it);
    }
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `union_by`: concat by unique keys (left order, then right-only keys). */
template <class Derived, class Iter>
template <class Rng, class KeyLeft, class KeyRight>
materialized_range<typename std::vector<typename query_range_algorithms<Derived, Iter>::value_type>::iterator,
    std::vector<typename query_range_algorithms<Derived, Iter>::value_type>>
query_range_algorithms<Derived, Iter>::union_by(
    Rng const& rhs, KeyLeft&& key_left, KeyRight&& key_right) const
{
    using val_t = typename query_range_algorithms<Derived, Iter>::value_type;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    using ref_l = typename query_range_algorithms<Derived, Iter>::reference;
    using rri = decltype(rhs.begin());
    using ref_r = decltype(*std::declval<rri>());
    static_assert(std::is_same_v<val_t, std::remove_cv_t<std::remove_reference_t<ref_r>>>,
        "qb::linq::union_by requires the same element type on both sides");
    using K = std::decay_t<std::invoke_result_t<KeyLeft&, ref_l>>;
    static_assert(std::is_same_v<K, std::decay_t<std::invoke_result_t<KeyRight&, ref_r>>>,
        "qb::linq::union_by: key types from left and right selectors must match");
    std::unordered_set<K> seen;
    auto owner = std::make_shared<std::vector<val_t>>();
    reserve_if_random_access(*owner, derived().begin(), derived().end());
    reserve_if_random_access(*owner, rhs.begin(), rhs.end());
    for (iterator_t it = derived().begin(); it != derived().end(); ++it) {
        K const k = key_left(*it);
        if (seen.insert(k).second)
            owner->push_back(*it);
    }
    for (auto it = rhs.begin(); it != rhs.end(); ++it) {
        K const k = key_right(*it);
        if (seen.insert(k).second)
            owner->push_back(*it);
    }
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `count_by`: (key, count) pairs in first-seen key order. */
template <class Derived, class Iter>
template <class KeyFn>
auto query_range_algorithms<Derived, Iter>::count_by(KeyFn&& keyf) const -> materialized_range<
    typename std::vector<std::pair<std::decay_t<std::invoke_result_t<KeyFn&,
                                           typename query_range_algorithms<Derived, Iter>::reference>>,
                                      int>>::iterator,
    std::vector<std::pair<std::decay_t<std::invoke_result_t<KeyFn&,
                                    typename query_range_algorithms<Derived, Iter>::reference>>,
                             int>>>
{
    using ref = typename query_range_algorithms<Derived, Iter>::reference;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    using K = std::decay_t<std::invoke_result_t<KeyFn&, ref>>;
    using row_t = std::pair<K, int>;
    using vec_t = std::vector<row_t>;
    std::unordered_map<K, int> counts;
    std::vector<K> order;
    reserve_if_random_access(counts, derived().begin(), derived().end());
    for (iterator_t it = derived().begin(); it != derived().end(); ++it) {
        K const k = keyf(*it);
        auto const r = counts.try_emplace(k, 0);
        if (r.second)
            order.push_back(k);
        ++r.first->second;
    }
    auto owner = std::make_shared<vec_t>();
    owner->reserve(order.size());
    for (K const& k : order)
        owner->emplace_back(k, counts.find(k)->second);
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `take_last`: tail `n` elements (random-access → copy only `min(n,len)` items). */
template <class Derived, class Iter>
materialized_range<typename std::vector<typename query_range_algorithms<Derived, Iter>::value_type>::iterator,
    std::vector<typename query_range_algorithms<Derived, Iter>::value_type>>
query_range_algorithms<Derived, Iter>::take_last(std::size_t n) const
{
    using val_t = typename query_range_algorithms<Derived, Iter>::value_type;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    if (n == 0) {
        auto owner = std::make_shared<std::vector<val_t>>();
        return wrap_materialized(std::move(owner));
    }
    iterator_t b = derived().begin();
    iterator_t e = derived().end();
    using cat = typename std::iterator_traits<iterator_t>::iterator_category;
    if constexpr (std::is_base_of_v<std::random_access_iterator_tag, cat>) {
        auto const dist = e - b;
        auto owner = std::make_shared<std::vector<val_t>>();
        if (dist <= 0)
            return wrap_materialized(std::move(owner));
        std::size_t const len = static_cast<std::size_t>(dist);
        std::size_t const take = (std::min)(n, len);
        using diff_t = typename std::iterator_traits<iterator_t>::difference_type;
        iterator_t const start = ::qb::linq::detail::ra_plus(b, static_cast<diff_t>(len - take));
        owner->reserve(take);
        for (iterator_t it = start; it != e; ++it)
            owner->push_back(*it);
        return wrap_materialized(std::move(owner));
    } else {
        std::deque<val_t> ring;
        for (iterator_t it = b; it != e; ++it) {
            if (ring.size() < n)
                ring.push_back(*it);
            else {
                ring.pop_front();
                ring.push_back(*it);
            }
        }
        auto owner = std::make_shared<std::vector<val_t>>(ring.begin(), ring.end());
        return wrap_materialized(std::move(owner));
    }
}

/** @brief Out-of-line `skip_last`: drop last `n` (random-access → copy only `len - n` elements). */
template <class Derived, class Iter>
materialized_range<typename std::vector<typename query_range_algorithms<Derived, Iter>::value_type>::iterator,
    std::vector<typename query_range_algorithms<Derived, Iter>::value_type>>
query_range_algorithms<Derived, Iter>::skip_last(std::size_t n) const
{
    using val_t = typename query_range_algorithms<Derived, Iter>::value_type;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    iterator_t b = derived().begin();
    iterator_t e = derived().end();
    using cat = typename std::iterator_traits<iterator_t>::iterator_category;
    if constexpr (std::is_base_of_v<std::random_access_iterator_tag, cat>) {
        auto owner = std::make_shared<std::vector<val_t>>();
        auto const dist = e - b;
        if (dist <= 0)
            return wrap_materialized(std::move(owner));
        std::size_t const len = static_cast<std::size_t>(dist);
        if (len <= n)
            return wrap_materialized(std::move(owner));
        std::size_t const keep = len - n;
        using diff_t = typename std::iterator_traits<iterator_t>::difference_type;
        iterator_t const end_keep = ::qb::linq::detail::ra_plus(b, static_cast<diff_t>(keep));
        owner->reserve(keep);
        for (iterator_t it = b; it != end_keep; ++it)
            owner->push_back(*it);
        return wrap_materialized(std::move(owner));
    } else {
        auto owner = std::make_shared<std::vector<val_t>>();
        reserve_if_random_access(*owner, b, e);
        for (iterator_t it = b; it != e; ++it)
            owner->push_back(*it);
        if (owner->size() > n)
            owner->erase(owner->end() - static_cast<typename std::vector<val_t>::difference_type>(n), owner->end());
        else
            owner->clear();
        return wrap_materialized(std::move(owner));
    }
}

/** @brief Out-of-line `join`: hash inner by key, emit `result(outer, inner)` for each match. */
template <class Derived, class Iter>
template <class Rng, class OuterKey, class InnerKey, class ResultSel>
auto query_range_algorithms<Derived, Iter>::join(Rng const& inner, OuterKey&& outer_key,
    InnerKey&& inner_key, ResultSel&& result) const
{
    using ref_t = typename query_range_algorithms<Derived, Iter>::reference;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    using inner_iter = decltype(inner.begin());
    using inner_ref_t = iter_reference_t<inner_iter>;
    using K = std::decay_t<std::invoke_result_t<OuterKey&, ref_t>>;
    static_assert(std::is_same_v<K, std::decay_t<std::invoke_result_t<InnerKey&, inner_ref_t>>>,
        "qb::linq::join: outer and inner key types must match");
    using InnerVal = std::decay_t<inner_ref_t>;
    using Row = std::decay_t<std::invoke_result_t<ResultSel&, ref_t, InnerVal const&>>;
    std::unordered_multimap<K, InnerVal> map;
    reserve_if_random_access(map, inner.begin(), inner.end());
    for (inner_iter it = inner.begin(); it != inner.end(); ++it)
        map.emplace(inner_key(*it), InnerVal(*it));
    auto owner = std::make_shared<std::vector<Row>>();
    reserve_if_random_access(*owner, derived().begin(), derived().end());
    for (iterator_t it = derived().begin(); it != derived().end(); ++it) {
        K const k = outer_key(*it);
        auto r = map.equal_range(k);
        for (auto in = r.first; in != r.second; ++in)
            owner->push_back(result(*it, in->second));
    }
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `group_join`: outer row paired with vector of matching inner elements. */
template <class Derived, class Iter>
template <class Rng, class OuterKey, class InnerKey>
auto query_range_algorithms<Derived, Iter>::group_join(
    Rng const& inner, OuterKey&& outer_key, InnerKey&& inner_key) const
{
    using ref_t = typename query_range_algorithms<Derived, Iter>::reference;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    using val_t = typename query_range_algorithms<Derived, Iter>::value_type;
    using inner_iter = decltype(inner.begin());
    using inner_ref_t = iter_reference_t<inner_iter>;
    using InnerVal = std::decay_t<inner_ref_t>;
    using K = std::decay_t<std::invoke_result_t<OuterKey&, ref_t>>;
    static_assert(std::is_same_v<K, std::decay_t<std::invoke_result_t<InnerKey&, inner_ref_t>>>,
        "qb::linq::group_join: outer and inner key types must match");
    using Row = std::pair<val_t, std::vector<InnerVal>>;
    std::unordered_multimap<K, InnerVal> map;
    reserve_if_random_access(map, inner.begin(), inner.end());
    for (inner_iter it = inner.begin(); it != inner.end(); ++it)
        map.emplace(inner_key(*it), InnerVal(*it));
    auto owner = std::make_shared<std::vector<Row>>();
    reserve_if_random_access(*owner, derived().begin(), derived().end());
    for (iterator_t it = derived().begin(); it != derived().end(); ++it) {
        K const k = outer_key(*it);
        std::vector<InnerVal> inners;
        auto r = map.equal_range(k);
        reserve_if_random_access(inners, r.first, r.second);
        for (auto in = r.first; in != r.second; ++in)
            inners.push_back(in->second);
        owner->emplace_back(static_cast<val_t>(*it), std::move(inners));
    }
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `left_join`: like `join`, plus one `nullopt` inner per unmatched outer. */
template <class Derived, class Iter>
template <class Rng, class OuterKey, class InnerKey, class ResultSel>
auto query_range_algorithms<Derived, Iter>::left_join(Rng const& inner, OuterKey&& outer_key,
    InnerKey&& inner_key, ResultSel&& result) const
{
    using ref_t = typename query_range_algorithms<Derived, Iter>::reference;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    using inner_iter = decltype(inner.begin());
    using inner_ref_t = iter_reference_t<inner_iter>;
    using K = std::decay_t<std::invoke_result_t<OuterKey&, ref_t>>;
    static_assert(std::is_same_v<K, std::decay_t<std::invoke_result_t<InnerKey&, inner_ref_t>>>,
        "qb::linq::left_join: outer and inner key types must match");
    using InnerVal = std::decay_t<inner_ref_t>;
    using Row = std::decay_t<std::invoke_result_t<ResultSel&, ref_t, std::optional<InnerVal> const&>>;
    std::unordered_multimap<K, InnerVal> map;
    reserve_if_random_access(map, inner.begin(), inner.end());
    for (inner_iter it = inner.begin(); it != inner.end(); ++it)
        map.emplace(inner_key(*it), InnerVal(*it));
    auto owner = std::make_shared<std::vector<Row>>();
    reserve_if_random_access(*owner, derived().begin(), derived().end());
    for (iterator_t it = derived().begin(); it != derived().end(); ++it) {
        K const k = outer_key(*it);
        auto r = map.equal_range(k);
        if (r.first == r.second) {
            std::optional<InnerVal> const empty{};
            owner->push_back(result(*it, empty));
        } else {
            for (auto in = r.first; in != r.second; ++in) {
                std::optional<InnerVal> const mo{in->second};
                owner->push_back(result(*it, mo));
            }
        }
    }
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `right_join`: index outer (`*this`); emit every inner, optional outer when unmatched. */
template <class Derived, class Iter>
template <class Rng, class OuterKey, class InnerKey, class ResultSel>
auto query_range_algorithms<Derived, Iter>::right_join(Rng const& inner, OuterKey&& outer_key,
    InnerKey&& inner_key, ResultSel&& result) const
{
    using ref_t = typename query_range_algorithms<Derived, Iter>::reference;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    using val_t = typename query_range_algorithms<Derived, Iter>::value_type;
    using inner_iter = decltype(inner.begin());
    using inner_ref_t = iter_reference_t<inner_iter>;
    using K = std::decay_t<std::invoke_result_t<OuterKey&, ref_t>>;
    static_assert(std::is_same_v<K, std::decay_t<std::invoke_result_t<InnerKey&, inner_ref_t>>>,
        "qb::linq::right_join: outer and inner key types must match");
    using InnerVal = std::decay_t<inner_ref_t>;
    using Row = std::decay_t<std::invoke_result_t<ResultSel&, std::optional<val_t> const&, InnerVal const&>>;
    std::unordered_multimap<K, val_t> map;
    reserve_if_random_access(map, derived().begin(), derived().end());
    for (iterator_t it = derived().begin(); it != derived().end(); ++it)
        map.emplace(outer_key(*it), static_cast<val_t>(*it));
    auto owner = std::make_shared<std::vector<Row>>();
    reserve_if_random_access(*owner, inner.begin(), inner.end());
    for (inner_iter it = inner.begin(); it != inner.end(); ++it) {
        InnerVal const inner_val(*it);
        K const k = inner_key(*it);
        auto r = map.equal_range(k);
        if (r.first == r.second) {
            std::optional<val_t> const empty{};
            owner->push_back(result(empty, inner_val));
        } else {
            for (auto out = r.first; out != r.second; ++out) {
                std::optional<val_t> const mo{out->second};
                owner->push_back(result(mo, inner_val));
            }
        }
    }
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `to_unordered_set`: unique elements in unspecified order. */
template <class Derived, class Iter>
auto query_range_algorithms<Derived, Iter>::to_unordered_set() const
{
    using val_t = typename query_range_algorithms<Derived, Iter>::value_type;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    using set_t = std::unordered_set<val_t>;
    auto owner = std::make_shared<set_t>();
    reserve_if_random_access(*owner, derived().begin(), derived().end());
    for (iterator_t it = derived().begin(); it != derived().end(); ++it)
        owner->insert(*it);
    return wrap_materialized(std::move(owner));
}

/** @brief Out-of-line `to_set`: unique elements in `operator<` order. */
template <class Derived, class Iter>
auto query_range_algorithms<Derived, Iter>::to_set() const
{
    using val_t = typename query_range_algorithms<Derived, Iter>::value_type;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    using set_t = std::set<val_t>;
    auto owner = std::make_shared<set_t>();
    for (iterator_t it = derived().begin(); it != derived().end(); ++it)
        owner->insert(*it);
    return wrap_materialized(std::move(owner));
}

} // namespace detail

} // namespace qb::linq

// ---- qb/linq/enumerable.h ----

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
 * `select`, `select_indexed`, `select_many`, `where`, `where_indexed`, `of_type`, `skip`, `skip_while`, `take`,
 * `take_while`, `skip_last`, `take_last`, `reverse`, `concat`, `zip` (two or three ranges), `default_if_empty`,
 * `enumerate`, `scan`, `chunk`, `stride`, `distinct`, `distinct_by`, `union_with`, `union_by`, `join`,
 * `left_join`, `right_join`, `group_join`, `to_lookup`, `group_by`, `order_by`, `except`, `intersect`,
 * `except_by`, `intersect_by`, `count_by`, map/set materializers, `append`, `prepend`.
 *
 * @par Terminals (eager)
 * `first`/`last`/`single` families, `element_at`, `contains`, `index_of`, `sequence_equal`, `count`,
 * `try_get_non_enumerated_count`, `any`/`all_if`/`none_if`, `sum`/`average`/min/max/`min_max`/`min_by`/`max_by`,
 * `aggregate`/`fold`/`reduce`,
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

    /** @brief Zip three sequences; stops at shortest length (`std::tuple` of references per step). */
    template <class Rng2, class Rng3>
    [[nodiscard]] auto zip(Rng2 const& r2, Rng3 const& r3) const
    {
        return pipe(static_cast<Handle const&>(*this).zip(r2, r3));
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

    /** @brief Left outer join: `result(outer, inner_optional)` with empty `optional` when no match. */
    template <class Rng, class OuterKey, class InnerKey, class ResultSel>
    [[nodiscard]] auto left_join(
        Rng const& inner, OuterKey&& ok, InnerKey&& ik, ResultSel&& rs) const
    {
        return pipe(static_cast<Handle const&>(*this).left_join(inner, std::forward<OuterKey>(ok),
            std::forward<InnerKey>(ik), std::forward<ResultSel>(rs)));
    }

    /** @brief Right outer join: `result(outer_optional, inner)` with empty `optional` when no outer match. */
    template <class Rng, class OuterKey, class InnerKey, class ResultSel>
    [[nodiscard]] auto right_join(
        Rng const& inner, OuterKey&& ok, InnerKey&& ik, ResultSel&& rs) const
    {
        return pipe(static_cast<Handle const&>(*this).right_join(inner, std::forward<OuterKey>(ok),
            std::forward<InnerKey>(ik), std::forward<ResultSel>(rs)));
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

    /** @brief `Except` by key (LINQ `ExceptBy`). */
    template <class Rng, class KeyLeft, class KeyRight>
    [[nodiscard]] auto except_by(Rng const& rhs, KeyLeft&& kl, KeyRight&& kr) const
    {
        return pipe(static_cast<Handle const&>(*this).except_by(
            rhs, std::forward<KeyLeft>(kl), std::forward<KeyRight>(kr)));
    }

    /** @brief `Intersect` by key (LINQ `IntersectBy`). */
    template <class Rng, class KeyLeft, class KeyRight>
    [[nodiscard]] auto intersect_by(Rng const& rhs, KeyLeft&& kl, KeyRight&& kr) const
    {
        return pipe(static_cast<Handle const&>(*this).intersect_by(
            rhs, std::forward<KeyLeft>(kl), std::forward<KeyRight>(kr)));
    }

    /** @brief `Union` by key (LINQ `UnionBy`); same `value_type` on both sides. */
    template <class Rng, class KeyLeft, class KeyRight>
    [[nodiscard]] auto union_by(Rng const& rhs, KeyLeft&& kl, KeyRight&& kr) const
    {
        return pipe(static_cast<Handle const&>(*this).union_by(
            rhs, std::forward<KeyLeft>(kl), std::forward<KeyRight>(kr)));
    }

    /** @brief Count per key, pairs in first-seen key order (LINQ `CountBy`). */
    template <class KeyFn>
    [[nodiscard]] auto count_by(KeyFn&& keyf) const
    {
        return pipe(static_cast<Handle const&>(*this).count_by(std::forward<KeyFn>(keyf)));
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

    /** @brief `Select` with index: `f(element, index)` (LINQ indexed overload). */
    template <class F>
    [[nodiscard]] auto select_indexed(F&& f) const
    {
        return pipe(static_cast<Handle const&>(*this).select_indexed(std::forward<F>(f)));
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

    /** @brief `Where` with index: `pred(element, index)`. */
    template <class P>
    [[nodiscard]] auto where_indexed(P&& p) const
    {
        return pipe(static_cast<Handle const&>(*this).where_indexed(std::forward<P>(p)));
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
    [[nodiscard]] auto take(std::ptrdiff_t n) const { return pipe(static_cast<Handle const&>(*this).take(n)); }

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
     * @brief Invokes `f` on each element (eager), then returns this pipeline stage for optional chaining.
     * @details The underlying `query_range_algorithms::each` runs the full pass before this returns; mutating
     *          `f` side effects apply even if you do not use the returned `enumerable`. The return value is
     *          only needed to continue chaining (e.g. `each(...).where(...)`).
     */
    template <class F>
    enumerable each(F&& f) const&
    {
        static_cast<Handle const&>(*this).each(std::forward<F>(f));
        return enumerable(static_cast<Handle const&>(*this));
    }

    /** @brief Non-`const` `enumerable&` overload of `each`. */
    template <class F>
    enumerable each(F&& f) &
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

    /** @brief O(1) size for random-access ranges only; otherwise `nullopt` (LINQ-style non-enumerated count). */
    [[nodiscard]] std::optional<std::size_t> try_get_non_enumerated_count() const
    {
        return static_cast<Handle const&>(*this).try_get_non_enumerated_count();
    }
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
