#ifndef FROM_H_
# define FROM_H_

namespace linq {
    template<typename Base>
    class basic_it : public Base {
    public:
        typedef Base base;
        typedef typename Base::iterator_category iterator_category;
        typedef typename Base::value_type value_type;
        typedef typename Base::difference_type difference_type;
        typedef typename Base::pointer pointer;
        typedef typename Base::reference reference;

        basic_it() = delete;
        basic_it(basic_it const&) = default;
        basic_it(Base const& base) noexcept
            : Base(base) {}

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
    };

    template<typename BaseIt>
    class From
        : public TState<basic_it<BaseIt>> {
    public:
        typedef basic_it<BaseIt> iterator;
        typedef iterator const_iterator;

        using base_t = TState<iterator>;
    public:
        ~From() = default;

        From() = delete;
        From(From const&) = default;
        From(BaseIt const& begin, BaseIt const& end) noexcept
            : base_t(begin, end) {}
    };
}

#endif // !FROM_H_
