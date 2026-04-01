#pragma once

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

#include <new>
#include <type_traits>
#include <utility>

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
