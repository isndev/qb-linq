#pragma once

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

#include <functional>
#include <map>
#include <type_traits>
#include <unordered_map>
#include <vector>

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
    static void emplace(type& vec, In const& val) { vec.push_back(static_cast<element_t>(val)); }
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
