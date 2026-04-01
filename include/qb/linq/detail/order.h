#pragma once

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

#include <type_traits>
#include <utility>

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
