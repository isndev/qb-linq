#ifndef UTILITY_H_
# define UTILITY_H_

namespace linq
{
    /* utils */

    template<typename Key, typename Value, bool is_basic_type>
    struct map_type
    {
        typedef std::unordered_map<Key, Value> type;
    };
    template<typename Key, typename Value>
    struct map_type<Key, Value, false>
    {
        typedef std::map<Key, Value> type;
    };

    template<typename In, typename KeyLoader = void, typename... Funcs>
    struct group_by
    {
        using Out = decltype(std::declval<KeyLoader>()(std::declval<In>()));
        typedef typename map_type<Out, typename group_by<In, Funcs...>::type, std::is_fundamental<Out>::value>::type type;

        static void emplace(type& handle, In const& val, KeyLoader const& func, Funcs const &...funcs) noexcept
        {
            group_by<In, Funcs...>::emplace(handle[func(val)], val, funcs...);
        }
    };
    template<typename In, typename KeyLoader>
    struct group_by<In, KeyLoader>
    {
        using Out = decltype(std::declval<KeyLoader>()(std::declval<In>()));
        typedef typename map_type<Out, typename group_by<In>::type, std::is_fundamental<Out>::value>::type type;

        static void emplace(type& handle, In const& val, KeyLoader const& func) noexcept
        {
            group_by<In>::emplace(handle[func(val)], val);
        }
    };
    template<typename In>
    struct group_by<In>
    {
        typedef std::vector<typename std::remove_reference<In>::type> type;

        static void emplace(type& vec, In const& val) noexcept
        {
            vec.push_back(val);
        }
    };

    // order utils

    // filter | filter type
    // modifier

    enum class eOrderType
    {
        asc,
        desc,
        custom
    };


    template<eOrderType FilterType, typename Key = void, typename = void>
    struct TFilter;

    struct FilterBase
    {
        template <typename Lhs, typename Rhs>
        bool next(Lhs const& lhs, Rhs const& rhs) const noexcept
        {
            return lhs == rhs;
        }
    };

    template<>
    struct TFilter<eOrderType::asc> : FilterBase
    {
        template <typename Lhs, typename Rhs>
        bool operator()(Lhs const& lhs, Rhs const& rhs) const noexcept
        {
            return lhs < rhs;
        }
    };

    template<>
    struct TFilter<eOrderType::desc> : FilterBase
    {
        template <typename Lhs, typename Rhs>
        bool operator()(Lhs const& lhs, Rhs const& rhs) const noexcept
        {
            return lhs > rhs;
        }
    };


    template<>
    struct TFilter<eOrderType::custom>
    {
        template <typename Lhs, typename Rhs>
        bool operator()(Lhs const& lhs, Rhs const& rhs) const noexcept
        {
            return false;
        }

        template <typename Lhs, typename Rhs>
        bool next(Lhs const& lhs, Rhs const& rhs) const noexcept
        {
            return false;
        }
    };

    template <typename BaseFilter, typename Key, typename ...Params>
    struct Filter : BaseFilter
    {
        explicit Filter(Key const& key, Params &&...params) : BaseFilter(std::forward<Params>(params)...), key_(key) {}

        template <typename Lhs, typename Rhs>
        bool apply(Lhs const& lhs, Rhs const& rhs) const
        {
            return (static_cast<BaseFilter const&>(*this))(key_(lhs), key_(rhs));
        }

        template <typename Lhs, typename Rhs>
        bool next(Lhs const& lhs, Rhs const& rhs) const
        {
            return (static_cast<BaseFilter const&>(*this)).next(key_(lhs), key_(rhs));
        }

    private:
        Key const& key_;
    };

    template<typename In, typename Filter, typename... Filters>
    bool order_by_current(In const& a, In const& b, Filter const& key, Filters &&...keys) noexcept
    {
        return key.apply(a, b) || (key.next(a, b) && order_by_current(a, b, std::forward<Filters>(keys)...));
    }
    template<typename In, typename Key>
    bool order_by_current(In const& a, In const& b, Key const& key) noexcept
    {
        return key.apply(a, b);
    }

    template <typename Key>
    using asc_t = Filter<TFilter<eOrderType::asc>, Key>;
    template <typename Key>
    using desc_t = Filter<TFilter<eOrderType::desc>, Key>;

    template <typename BaseFilter, typename Key, typename ...Params>
    auto make_filter(Key const& key, Params &&...params) noexcept
    {
        return Filter<BaseFilter, Key, Params...>(key, std::forward<Params>(params)...);
    }
    template <typename Key>
    auto asc(Key const& key) noexcept { return asc_t<Key>(key); }
    template <typename Key>
    auto desc(Key const& key) noexcept { return desc_t<Key>(key); }

    /*! utils */
}

#endif // !UTILITY_H_
