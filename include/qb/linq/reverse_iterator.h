#pragma once

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

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

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
