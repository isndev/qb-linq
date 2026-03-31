#pragma once

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
 * `query_range.h` (declarations) → `iterators.h` → `extra_views.h` (definitions) → template bodies below for
 * `group_by`, `order_by`, `to_*`, `join`, set ops (require complete `materialized_range`).
 *
 * @par Types in this header
 * | Type | Role |
 * |------|------|
 * | `query_state` | Minimal `[begin,end)` + CRTP algorithms |
 * | `reversed_view` | `std::reverse_iterator` adapter |
 * | `subrange` | Slice after `skip` / `skip_while` |
 * | `from_range` | `basic_iterator` wrapper for raw iterators |
 * | `select_view` / `where_view` / `take_n_view` / `take_while_view` | Lazy views |
 * | `iota_iterator` / `iota_view` | Arithmetic sequence (no container) |
 * | `materialized_range` | `shared_ptr` to owner + iterators |
 */

#include <algorithm>
#include <cstddef>
#include <deque>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "qb/linq/detail/query_range.h"
#include "qb/linq/iterators.h"
#include "qb/linq/detail/extra_views.h"

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
 * @brief Reverse iteration over `[src.begin(), src.end())` using `std::reverse_iterator`.
 * @tparam Iter Underlying iterator type from the adapted range.
 */
template <class Iter>
class reversed_view : public query_state<std::reverse_iterator<Iter>> {
    using base_t = query_state<std::reverse_iterator<Iter>>;

public:
    /**
     * @brief Adapts `src` so iteration runs from `src.end()` toward `src.begin()` (bidirectional requirement on `Rng`).
     * @tparam Rng Any type with `begin`/`end` yielding iterators convertible from `Iter`’s source category.
     * @param src Range to traverse in reverse order.
     */
    template <class Rng, class = std::void_t<decltype(std::declval<Rng>().begin())>>
    explicit reversed_view(Rng const& src)
        : base_t(std::reverse_iterator<Iter>(src.end()), std::reverse_iterator<Iter>(src.begin()))
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
    mutable std::optional<BaseIt> first_{};

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
        if (!first_.has_value())
            first_.emplace(std::find_if(b_, e_, pred_));
        return iterator(first_.value(), b_, e_, pred_);
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
    take_n_view(BaseIt b, BaseIt e, int n)
        : query_state<take_n_iterator<BaseIt>>(
              take_n_iterator<BaseIt>(std::move(b), -take_n_count(n)),
              take_n_iterator<BaseIt>(std::move(e), 0))
    {}

private:
    /**
     * @brief Magnitude of `n` for take count (`n < 0` → `abs`); overflow / `INT_MIN` → `0`.
     * @see take_n_iterator::remaining_
     */
    static int take_n_count(int n) noexcept
    {
        long long const v = n;
        long long const mag = v < 0 ? -v : v;
        if (mag > static_cast<long long>(std::numeric_limits<int>::max()))
            return 0;
        return static_cast<int>(mag);
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
     * @brief Iterator range over data owned by `own` (shared lifetime).
     * @param b Iterator to first element in `*own` (or map begin).
     * @param e Iterator past the exposed range.
     * @param own Shared ownership; must outlive any iterator derived from this range.
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

    /** @brief `std::reverse_iterator` view over the same stored `[begin,end)` span. */
    [[nodiscard]] materialized_range<std::reverse_iterator<Iter>, Owner> reverse() const
    {
        return materialized_range<std::reverse_iterator<Iter>, Owner>(
            std::reverse_iterator<Iter>(this->end_), std::reverse_iterator<Iter>(this->begin_), owner_);
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
 * @brief Template definitions below: `group_by`, `order_by`, container materializers, `join` / `group_join`,
 *        set-like ops. Declarations live in `query_range.h`.
 */
namespace detail {

/** @brief Out-of-line `group_by`: populate nested map, return iterators over buckets. */
template <class Derived, class Iter>
template <class... Keys>
materialized_range<typename group_by_map_t<typename query_range_algorithms<Derived, Iter>::reference, Keys...>::iterator,
    group_by_map_t<typename query_range_algorithms<Derived, Iter>::reference, Keys...>>
query_range_algorithms<Derived, Iter>::group_by(Keys&&... keys) const
{
    using ref = typename query_range_algorithms<Derived, Iter>::reference;
    using map_t = group_by_map_t<ref, Keys...>;
    using iterator_t = typename query_range_algorithms<Derived, Iter>::iterator;
    auto owner = std::make_shared<map_t>();
    for (iterator_t it = derived().begin(); it != derived().end(); ++it)
        group_by_impl<ref, std::decay_t<Keys>...>::emplace(*owner, *it, keys...);
    return materialized_range<typename map_t::iterator, map_t>(owner->begin(), owner->end(), std::move(owner));
}

/** @brief Out-of-line `order_by`: copy to vector, `std::sort` with lexicographic key extractors. */
template <class Derived, class Iter>
template <class... KeyFilters>
materialized_range<typename std::vector<typename query_range_algorithms<Derived, Iter>::value_type>::iterator,
    std::vector<typename query_range_algorithms<Derived, Iter>::value_type>>
query_range_algorithms<Derived, Iter>::order_by(KeyFilters&&... key_filters) const
{
    using val_t = typename query_range_algorithms<Derived, Iter>::value_type;
    using ref_t = typename query_range_algorithms<Derived, Iter>::reference;
    auto owner = std::make_shared<std::vector<val_t>>();
    reserve_if_random_access(*owner, derived().begin(), derived().end());
    std::copy(derived().begin(), derived().end(), std::back_inserter(*owner));
    std::sort(owner->begin(), owner->end(), [&](ref_t a, ref_t b) {
        return lexicographic_compare(a, b, key_filters...);
    });
    return materialized_range<typename std::vector<val_t>::iterator, std::vector<val_t>>(
        owner->begin(), owner->end(), std::move(owner));
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
    return materialized_range<typename std::vector<val_t>::iterator, std::vector<val_t>>(
        owner->begin(), owner->end(), std::move(owner));
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
    return materialized_range<typename map_t::iterator, map_t>(
        owner->begin(), owner->end(), std::move(owner));
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
    return materialized_range<typename map_t::iterator, map_t>(
        owner->begin(), owner->end(), std::move(owner));
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
    return materialized_range<typename map_t::iterator, map_t>(
        owner->begin(), owner->end(), std::move(owner));
}

/** @brief Out-of-line `except`: filter out values present in `rhs` (unordered set of `value_type`). */
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
    auto owner = std::make_shared<std::vector<val_t>>();
    reserve_if_random_access(*owner, derived().begin(), derived().end());
    for (iterator_t it = derived().begin(); it != derived().end(); ++it) {
        if (ban.find(*it) == ban.end())
            owner->push_back(*it);
    }
    return materialized_range<typename std::vector<val_t>::iterator, std::vector<val_t>>(
        owner->begin(), owner->end(), std::move(owner));
}

/** @brief Out-of-line `intersect`: keep elements also in `rhs` (order follows this sequence). */
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
    auto owner = std::make_shared<std::vector<val_t>>();
    reserve_if_random_access(*owner, derived().begin(), derived().end());
    for (iterator_t it = derived().begin(); it != derived().end(); ++it) {
        if (want.find(*it) != want.end())
            owner->push_back(*it);
    }
    return materialized_range<typename std::vector<val_t>::iterator, std::vector<val_t>>(
        owner->begin(), owner->end(), std::move(owner));
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
        return materialized_range<typename std::vector<val_t>::iterator, std::vector<val_t>>(
            owner->begin(), owner->end(), std::move(owner));
    }
    iterator_t b = derived().begin();
    iterator_t e = derived().end();
    using cat = typename std::iterator_traits<iterator_t>::iterator_category;
    if constexpr (std::is_base_of_v<std::random_access_iterator_tag, cat>) {
        auto const dist = e - b;
        auto owner = std::make_shared<std::vector<val_t>>();
        if (dist <= 0)
            return materialized_range<typename std::vector<val_t>::iterator, std::vector<val_t>>(
                owner->begin(), owner->end(), std::move(owner));
        std::size_t const len = static_cast<std::size_t>(dist);
        std::size_t const take = (std::min)(n, len);
        using diff_t = typename std::iterator_traits<iterator_t>::difference_type;
        iterator_t const start = ::qb::linq::detail::ra_plus(b, static_cast<diff_t>(len - take));
        owner->reserve(take);
        for (iterator_t it = start; it != e; ++it)
            owner->push_back(*it);
        return materialized_range<typename std::vector<val_t>::iterator, std::vector<val_t>>(
            owner->begin(), owner->end(), std::move(owner));
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
        return materialized_range<typename std::vector<val_t>::iterator, std::vector<val_t>>(
            owner->begin(), owner->end(), std::move(owner));
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
            return materialized_range<typename std::vector<val_t>::iterator, std::vector<val_t>>(
                owner->begin(), owner->end(), std::move(owner));
        std::size_t const len = static_cast<std::size_t>(dist);
        if (len <= n)
            return materialized_range<typename std::vector<val_t>::iterator, std::vector<val_t>>(
                owner->begin(), owner->end(), std::move(owner));
        std::size_t const keep = len - n;
        using diff_t = typename std::iterator_traits<iterator_t>::difference_type;
        iterator_t const end_keep = ::qb::linq::detail::ra_plus(b, static_cast<diff_t>(keep));
        owner->reserve(keep);
        for (iterator_t it = b; it != end_keep; ++it)
            owner->push_back(*it);
        return materialized_range<typename std::vector<val_t>::iterator, std::vector<val_t>>(
            owner->begin(), owner->end(), std::move(owner));
    } else {
        auto owner = std::make_shared<std::vector<val_t>>();
        reserve_if_random_access(*owner, b, e);
        for (iterator_t it = b; it != e; ++it)
            owner->push_back(*it);
        if (owner->size() > n)
            owner->erase(owner->end() - static_cast<typename std::vector<val_t>::difference_type>(n), owner->end());
        else
            owner->clear();
        return materialized_range<typename std::vector<val_t>::iterator, std::vector<val_t>>(
            owner->begin(), owner->end(), std::move(owner));
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
    return materialized_range<typename std::vector<Row>::iterator, std::vector<Row>>(
        owner->begin(), owner->end(), std::move(owner));
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
    return materialized_range<typename std::vector<Row>::iterator, std::vector<Row>>(
        owner->begin(), owner->end(), std::move(owner));
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
    return materialized_range<typename set_t::iterator, set_t>(
        owner->begin(), owner->end(), std::move(owner));
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
    return materialized_range<typename set_t::iterator, set_t>(owner->begin(), owner->end(), std::move(owner));
}

} // namespace detail

} // namespace qb::linq
