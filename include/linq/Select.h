#ifndef SELECT_H_
# define SELECT_H_

namespace linq {
    template<typename T, bool value = std::is_reference<T>::value>
    struct is_ref {
        typedef T type;
    };

    template<typename T>
    struct is_ref<T, false> {
        typedef std::remove_reference_t<T> type;
    };

    template<typename Base, typename Loader>
    class select_it : public Base {
    public:
        typedef Base base;
        typedef decltype(std::declval<Loader>()(*std::declval<Base>())) original_type;
        typedef typename Base::iterator_category iterator_category;
        typedef typename std::remove_reference<original_type>::type value_type;
        typedef typename Base::difference_type difference_type;
        typedef value_type* pointer;
        typedef typename is_ref<original_type>::type reference;

        select_it() = delete;
        select_it(select_it const& rhs) = default;
        select_it(Base const& base, Loader const& loader) noexcept
            : Base(base), loader_(loader) {}

        auto& operator=(select_it const& rhs) noexcept {
            static_cast<Base>(*this) = static_cast<Base const&>(rhs);
            return *this;
        }
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
        const reference operator*() const noexcept {
            return loader_(*static_cast<Base const&>(*this));
        }
        reference operator*() noexcept {
            return loader_(*static_cast<Base&>(*this));
        }

    private:
        Loader loader_;
    };

    template<typename BaseIt, typename Loader>
    class Select
        : public TState<select_it<BaseIt, Loader>> {
    public:
        typedef select_it<BaseIt, Loader> iterator;
        typedef iterator const_iterator;

        using base_t = TState<iterator>;
    public:
        ~Select() = default;

        Select() = delete;
        Select(Select const& rhs) noexcept
            : base_t(static_cast<base_t const&>(rhs)) {}
        Select(BaseIt const& begin, BaseIt const& end, Loader const& loader)
            : base_t(iterator(begin, loader), iterator(end, loader)) {}
    };
}

#endif // !SELECT_H_
