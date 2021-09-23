#ifndef TAKE_H_
# define TAKE_H_

namespace linq {
    template<typename Base, typename In>
    class take_it : public Base {
    public:
        typedef Base base;
        typedef typename Base::iterator_category iterator_category;
        typedef typename Base::value_type value_type;
        typedef typename Base::difference_type difference_type;
        typedef typename Base::pointer pointer;
        typedef typename Base::reference reference;

        take_it() = delete;
        take_it(const take_it&) = default;
        take_it(Base const& base, In const& when) noexcept : Base(base), _when(when) {}

        auto& operator++() noexcept {
            static_cast<Base&>(*this).operator++();
            return (*this);
        }
        auto operator++(int) noexcept {
            auto tmp = *this;
            operator++();
            return (tmp);
        }
        auto& operator--() noexcept {
            static_cast<Base&>(*this).operator--();
            return (*this);
        }
        auto operator--(int) noexcept {
            auto tmp = *this;
            operator--();
            return (tmp);
        }
        bool operator!=(take_it const& rhs) const noexcept {
            return static_cast<Base const&>(*this) != static_cast<Base const&>(rhs)
                && _when(*static_cast<Base const&>(*this));
        }
        bool operator==(take_it const& rhs) const noexcept {
            return static_cast<Base const&>(*this) == static_cast<Base const&>(rhs)
                || !_when(*static_cast<Base const&>(rhs));
        }

    private:
        In const _when;
    };

    template<typename Base>
    class take_it<Base, int> : public Base {
    public:
        typedef Base base;
        typedef typename Base::iterator_category iterator_category;
        typedef typename Base::value_type value_type;
        typedef typename Base::difference_type difference_type;
        typedef typename Base::pointer pointer;
        typedef typename Base::reference reference;

        take_it() = delete;
        take_it(const take_it&) = default;
        take_it(Base const& base, int const max) noexcept : Base(base), max_(-max) {}

        auto& operator++() noexcept {
            ++max_;
            static_cast<Base&>(*this).operator++();
            return (*this);
        }
        auto operator++(int) noexcept {
            auto tmp = *this;
            operator++();
            ++max_;
            return (tmp);
        }
        auto& operator--() noexcept {
            --max_;
            static_cast<Base&>(*this).operator--();
            return (*this);
        }
        auto operator--(int) noexcept {
            auto tmp = *this;
            operator--();
            --max_;
            return (tmp);
        }
        bool operator!=(take_it const& rhs) const noexcept {
            return static_cast<Base const&>(*this) != static_cast<Base const&>(rhs) && max_ != rhs.max_;
        }
        bool operator==(take_it const& rhs) const noexcept {
            return static_cast<Base const&>(*this) == static_cast<Base const&>(rhs) || max_ == rhs.max_;
        }

    private:
        mutable int max_;
    };

    template<typename Base, typename In = int>
    class Take : public TState<take_it<Base, In>> {
    public:
        typedef take_it<Base, In> iterator;
        typedef iterator const_iterator;

        using base_t = TState<iterator>;
    public:
        ~Take() = default;

        Take() = delete;
        Take(Take const& rhs)
            : base_t(static_cast<base_t const&>(rhs)) {}
        Take(Base const& begin, Base const& end, In const& in) noexcept
            : base_t(iterator(begin, in), iterator(end, in)) {}

    };

    template<typename Base>
    class Take<Base, int> : public TState<take_it<Base, int>> {
    public:
        typedef take_it<Base, int> iterator;
        typedef iterator const_iterator;

        using base_t = TState<iterator>;
    public:
        ~Take() = default;

        Take() = delete;
        Take(Take const& rhs)
            : base_t(static_cast<base_t const&>(rhs)) {}
        Take(Base const& begin, Base const& end, int const& in) noexcept
            : base_t(iterator(begin, in), iterator(end, 0)) {}

    };
}

#endif // !TAKE_H_
