#ifndef WHERE_H_
# define WHERE_H_

namespace linq
{
    template <typename Base, typename Filter>
    class where_it : public Base {
    public:
        typedef Base base;
        typedef typename Base::iterator_category                iterator_category;
        typedef decltype(*std::declval<Base>())                    value_type;
        typedef typename Base::difference_type                    difference_type;
        typedef typename Base::pointer                            pointer;
        typedef value_type                                         reference;

        where_it() = delete;
        where_it(where_it const &) = default;
        where_it(Base const &base, Base const &begin, Base const &end, Filter const &filter) noexcept(true)
                : Base(base), begin_(begin), end_(end), filter_(filter) {}

        constexpr auto const &operator=(where_it const &rhs) noexcept(true) {
            static_cast<Base>(*this) = static_cast<Base const &>(rhs);
            return *this;
        }
        constexpr auto const &operator++() noexcept(true) {
            do
            {
                static_cast<Base &>(*this).operator++();
            } while (static_cast<Base const &>(*this) != end_ && !filter_(*static_cast<Base const &>(*this)));
            return *this;
        }
        constexpr auto operator++(int) noexcept(true)
        {
            auto tmp = *this;
            operator++();
            return (tmp);
        }
        constexpr auto const &operator--() noexcept(true) {
            do
            {
                static_cast<Base &>(*this).operator--();
            } while (static_cast<Base const &>(*this) != begin_ && !filter_(*static_cast<Base const &>(*this)));
            return *this;
        }
        constexpr auto operator--(int) noexcept(true)
        {
            auto tmp = *this;
            operator--();
            return (tmp);
        }

    private:
        Base const begin_;
        Base const end_;
        Filter const filter_;
    };

    template<typename BaseIt, typename Filter>
    class Where : public TState<where_it<BaseIt, Filter>> {
    public:
        typedef where_it<BaseIt, Filter> iterator;
        typedef iterator const_iterator;

        using base_t = TState<iterator>;
    public:
        ~Where() = default;
        Where() = delete;
        Where(Where const &rhs)
                : base_t(static_cast<base_t const &>(rhs))
        {}
        Where(BaseIt const &begin, BaseIt const &end, Filter const &filter) noexcept(true)
                : base_t(iterator(begin, begin, end, filter), iterator(end, begin, end, filter))
        {}
    };
}

#endif // !WHERE_H_
