#ifndef ISTATE_H_
# define ISTATE_H_

namespace linq
{
    template<typename Iterator>
    class TState
    {
        using Out = typename Iterator::value_type;
    protected:
        Iterator const begin_;
        Iterator const end_;

    public:
        ~TState() = default;
        TState() = delete;
        TState(TState const &rhs) = default;
        TState(Iterator const &&begin, Iterator const &&end)
                : begin_(begin), end_(end) {}

        constexpr Iterator const &begin() const noexcept(true) { return begin_; }
        constexpr Iterator const &end() const noexcept(true) { return end_; }
        constexpr auto rbegin() const noexcept(true) { return std::reverse_iterator<Iterator>(begin_); }
        constexpr auto rend() const noexcept(true) { return std::reverse_iterator<Iterator>(end_); }

        constexpr Out first() const noexcept(true) { return *begin_; }
        constexpr auto firstOrDefault() const noexcept(true) {
            return any() ? first() : typename std::remove_reference<Out>::type{};
        }

        constexpr Out last() const noexcept(true) { return *std::reverse_iterator<Iterator>(end_); }
        constexpr auto lastOrDefault() const noexcept(true) {
            return any() ? last() : typename std::remove_reference<Out>::type{};
        }

        constexpr auto reverse() const noexcept(true) {
            return From<std::reverse_iterator<Iterator>>(rend(), rbegin());
        }

        template<typename Func>
        constexpr auto select(Func const &nextloader_) const noexcept(true) {
            return Select<Iterator, Func>(
                    this->begin_,
                    this->end_,
                    nextloader_);
        }
        template<typename... Funcs>
        constexpr auto selectMany(Funcs const &...loaders) const noexcept(true) {
            auto const &nextloader_ = [loaders...] (Out val)
            {
                return std::tuple<decltype(loaders(val))...>(loaders(val)...);
            };

            return Select<Iterator, decltype(nextloader_)>(
                    this->begin_,
                    this->end_,
                    nextloader_);
        }

        template<typename Func>
        constexpr auto where(Func const &nextfilter_) const noexcept(true) {
            auto const newbegin_ = std::find_if(begin_, end_, nextfilter_);
            return Where<Iterator, Func>(newbegin_,
                                         any() ? std::find_if(std::reverse_iterator<Iterator>(this->end_),
                                                              std::reverse_iterator<Iterator>(newbegin_),
                                                              nextfilter_).base() : end_,
                                         nextfilter_
            );
        }
        template<typename... Funcs>
        constexpr auto groupBy(Funcs const &...keys) const noexcept(true) {
            using group_type = group_by<Out, Funcs...>;
            using map_out = typename group_type::type;
            auto result = std::make_shared<map_out>();

            for (Out it : *this)
                group_type::emplace(*result, it, keys...);

            return All<typename map_out::iterator, decltype(result)>(result->begin(), result->end(), result);
        }
        template<typename... Funcs>
        constexpr auto orderBy(Funcs const &... keys) const noexcept(true) {
            auto proxy = std::make_shared<std::vector<typename std::remove_reference<Out>::type>>();
            std::copy(begin_, end_, std::back_inserter(*proxy));
            std::sort(proxy->begin(), proxy->end(), [keys...](Out a, Out b) -> bool
            {
                return order_by_current(a, b, keys...);
            });

            return All<decltype(proxy->begin()), decltype(proxy)>(proxy->begin(), proxy->end(), proxy);
        }

        constexpr auto skip(std::size_t const offset) const noexcept(true) {
            auto ret = begin_;
            for (std::size_t i = 0; i < offset && ret != end_; ++ret, ++i);

            return From<Iterator>(ret, end_);
        }
        template<typename Func>
        constexpr auto skip_while(Func const &func) const noexcept(true) {
            auto ret = begin_;
            while (ret != end_ && func(*ret))
                ++ret;
            return From<Iterator>(ret, end_);
        }

        constexpr auto take(int const max) const noexcept(true) {
            return Take<Iterator>(begin_, end_, max);
        }
        template<typename Func>
        constexpr auto take_while(Func const &func) const noexcept(true) {
            return Take<Iterator, Func>(begin_, end_, func);
        }

        template<typename Func>
        constexpr void each(Func const &pred) const noexcept(true) {
            for (Out it : *this)
                pred(it);
        }

        template <typename T>
        constexpr bool contains(T const &elem) const noexcept(true)
        {
            for (Out it : *this)
                if (it == elem)
                    return true;
            return false;
        }
        constexpr bool any() const noexcept(true) {
            return begin_ != end_;
        }
        constexpr auto count() const noexcept(true) {
            std::size_t number{ 0 };
            for (auto it = begin_; it != end_; ++it, ++number);
            return number;
        }

        constexpr auto all() const noexcept(true)
        {
            using vec_out = typename std::vector<typename std::remove_const<typename std::remove_reference<Out>::type>::type>;
            auto proxy = std::make_shared<vec_out>();
            for (Out it : *this)
                proxy->push_back(it);
            return All<typename vec_out::iterator, decltype(proxy)>(proxy->begin(), proxy->end(), proxy);
        }
        constexpr auto min() const noexcept(true) {
            typename std::remove_const<typename std::remove_reference<decltype(*begin_)>::type>::type val(*begin_);
            for (Out it : *this)
                if (it > val)
                    val = it;
            return val;
        }
        constexpr auto max() const noexcept(true) {
            typename std::remove_const<typename std::remove_reference<decltype(*begin_)>::type>::type val(*begin_);
            for (Out it : *this)
                if (it < val)
                    val = it;
            return val;
        }
        constexpr auto sum() const noexcept(true) {
            typename std::remove_const<typename std::remove_reference<Out>::type>::type result{};
            for (Out it : *this)
                result += it;
            return result;
        }

    };
}

#endif // !ISTATE_H_
