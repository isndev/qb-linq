#ifndef ISTATE_H_
# define ISTATE_H_

namespace linq {
    template<typename Iterator>
    class TState {
        using reference = typename Iterator::reference;
    protected:
        Iterator const begin_;
        Iterator const end_;

    public:
        ~TState() = default;

        TState() = delete;
        TState(TState const& rhs) = default;
        TState(Iterator const& begin, Iterator const& end)
            : begin_(begin), end_(end) {}

        Iterator begin() const noexcept { return begin_; }
        Iterator end() const noexcept { return end_; }
        auto rbegin() const noexcept { return std::reverse_iterator<Iterator>(begin_); }
        auto rend() const noexcept { return std::reverse_iterator<Iterator>(end_); }
        auto first() const noexcept { return *begin_; }
        auto firstOrDefault() const noexcept {
            return any() ? first() : typename std::remove_reference<reference>::type{};
        }
        auto last() const noexcept { return *std::reverse_iterator<Iterator>(end_); }
        auto lastOrDefault() const noexcept {
            return any() ? last() : typename std::remove_reference<reference>::type{};
        }

        auto reverse() const noexcept {
            return From<std::reverse_iterator<Iterator>>(rend(), rbegin());
        }

        template<typename Func>
        auto select(Func&& nextloader_) const noexcept {
            return Select<Iterator, Func>(
                this->begin_,
                this->end_,
                std::forward<Func>(nextloader_));
        }

        template<typename... Funcs>
        auto selectMany(Funcs &&...loaders) const noexcept {
            auto const& nextloader_ = [&loaders...](reference val) {
                return std::tuple<decltype(loaders(val))...>(loaders(val)...);
            };

            return Select<Iterator, decltype(nextloader_)>(
                this->begin_,
                this->end_,
                nextloader_);
        }

        template<typename Func>
        auto where(Func&& nextfilter_) const noexcept {
            auto const newbegin_ = std::find_if(begin_, end_, std::forward<Func>(nextfilter_));
            return Where<Iterator, Func>(newbegin_,
                any() ? std::find_if(std::reverse_iterator<Iterator>(this->end_),
                    std::reverse_iterator<Iterator>(newbegin_),
                    std::forward<Func>(nextfilter_)).base() : end_,
                std::forward<Func>(nextfilter_)
                );
        }

        template<typename... Funcs>
        auto groupBy(Funcs &&...keys) const noexcept {
            using group_type = group_by<reference, Funcs...>;
            using map_out = typename group_type::type;
            auto result = std::make_shared<map_out>();

            for (const reference it : *this)
                group_type::emplace(*result, it, std::forward<Funcs>(keys)...);

            return All<typename map_out::iterator, decltype(result)>(result->begin(), result->end(), result);
        }

        template<typename... Funcs>
        auto orderBy(Funcs &&... keys) const noexcept {
            auto proxy = std::make_shared<std::vector<typename std::remove_reference<reference>::type>>();
            std::copy(begin_, end_, std::back_inserter(*proxy));
            std::sort(proxy->begin(), proxy->end(), [&keys...](reference a, reference b) -> bool {
                return order_by_current(a, b, std::forward<Funcs>(keys)...);
            });

            return All<decltype(proxy->begin()), decltype(proxy)>(proxy->begin(), proxy->end(), proxy);
        }

        auto skip(std::size_t const offset) const noexcept {
            auto ret = begin_;
            for (std::size_t i = 0; i < offset && ret != end_; ++ret, ++i);

            return From<Iterator>(ret, end_);
        }

        template<typename Func>
        auto skip_while(Func&& func) const noexcept {
            auto ret = begin_;
            while (ret != end_ && func(std::cref(*ret)))
                ++ret;
            return From<Iterator>(ret, end_);
        }

        auto take(int const max) const noexcept {
            return Take<Iterator>(begin_, end_, max);
        }

        template<typename Func>
        auto take_while(Func&& func) const noexcept {
            return Take<Iterator, Func>(begin_, end_, func);
        }

        template<typename Func>
        void each(Func&& pred) noexcept {
            for (reference it : *this)
                pred(it);
        }

        template<typename Func>
        void each(Func&& pred) const noexcept {
            for (const reference it : *this)
                pred(it);
        }

        template<typename T>
        bool contains(T const& elem) const noexcept {
            for (const reference it : *this)
                if (it == elem)
                    return true;
            return false;
        }

        bool any() const noexcept {
            return begin_ != end_;
        }

        auto count() const noexcept {
            std::size_t number{ 0 };
            for (auto it = begin_; it != end_; ++it, ++number);
            return number;
        }

        auto all() const noexcept {
            using vec_out = typename std::vector<typename std::remove_const<typename std::remove_reference<reference>::type>::type>;
            auto proxy = std::make_shared<vec_out>();
            for (const reference it : *this)
                proxy->push_back(it);
            return All<typename vec_out::iterator, decltype(proxy)>(proxy->begin(), proxy->end(), proxy);
        }

        auto min() const noexcept {
            typename std::remove_const<typename std::remove_reference<decltype(*begin_)>::type>::type val(*begin_);
            for (const reference it : *this)
                if (it > val)
                    val = it;
            return val;
        }

        auto max() const noexcept {
            typename std::remove_const<typename std::remove_reference<decltype(*begin_)>::type>::type val(*begin_);
            for (const reference it : *this)
                if (it < val)
                    val = it;
            return val;
        }

        auto sum() const noexcept {
            typename std::remove_const<typename std::remove_reference<reference>::type>::type result{};
            auto i = 0;
            for (const reference it : *this) {
                result += it;
                ++i;
            }
            return result;
        }

    };
}

#endif // !ISTATE_H_
