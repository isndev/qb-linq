#ifndef ALL_H_
# define ALL_H_

namespace linq
{
    template <typename Base, typename Proxy>
    class all_it : public Base {
    public:
        typedef Base base;
        typedef typename Base::iterator_category iterator_category;
        typedef typename Base::value_type value_type;
        typedef typename Base::difference_type difference_type;
        typedef typename Base::pointer pointer;
        typedef typename Base::reference reference;

        all_it() = delete;
        all_it(all_it const&) = default;
        all_it(Base const& base, Proxy proxy) noexcept
            : Base(base), proxy_(proxy)
        {}

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

    private:
        Proxy proxy_;
    };

    template<typename BaseIt, typename Proxy>
    class All : public TState<all_it<BaseIt, Proxy>> {
    public:
        typedef all_it<BaseIt, Proxy> iterator;
        typedef iterator const_iterator;

        using base_t = TState<iterator>;
    public:
        ~All() = default;
        All() = delete;
        All(All const&) = default;
        All(BaseIt const& begin, BaseIt const& end, Proxy proxy) noexcept
            : base_t(iterator(begin, proxy), iterator(end, proxy)), proxy_(proxy)
        {}

        auto asc() const noexcept { return *this; }
        auto desc() const noexcept {
            return All<decltype(proxy_->rbegin()), Proxy>(proxy_->rbegin(), proxy_->rend(), proxy_);
        }

        template<typename Key>
        auto& operator[](Key const& key) const
        {
            return (*proxy_).at(key);
        }

    private:
        Proxy proxy_;
    };
}

#endif // !All_H_
