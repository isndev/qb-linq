#pragma once

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

#include <iterator>
#include <type_traits>
#include <utility>

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
    [[nodiscard]] std::decay_t<T> operator()(T&& x) const
    {
        return std::forward<T>(x);
    }
    /** @} */
};

} // namespace qb::linq::detail
