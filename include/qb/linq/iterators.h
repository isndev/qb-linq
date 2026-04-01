#pragma once

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

#include <algorithm>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>

#include "qb/linq/detail/type_traits.h"

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
    take_n_iterator(BaseIt base, int remaining) noexcept(
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
    int remaining_;
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
