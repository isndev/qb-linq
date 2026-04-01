#pragma once

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
 * | `default_if_empty_iterator` / `default_if_empty_view` | Default value if source empty |
 * | `distinct_iterator` / `distinct_view` | First occurrence per key (hash set) |
 * | `enumerate_iterator` / `enumerate_view` | `(index, element)` |
 * | `chunk_iterator` / `chunk_view` | Slices as `std::vector` chunks |
 * | `stride_iterator` / `stride_view` | Every k-th element |
 * | `scan_iterator` / `scan_view` | Inclusive prefix fold |
 */
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "qb/linq/detail/query_range.h"

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
            return const_cast<reference>(parent_->default_value_ref());
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
    T def_{};

    [[nodiscard]] T const& default_value_ref() const noexcept { return def_; }

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
