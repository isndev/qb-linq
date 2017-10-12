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

		constexpr static void emplace(type &handle, In const &val, KeyLoader const &func, Funcs const &...funcs) noexcept(true)
		{
			group_by<In, Funcs...>::emplace(handle[func(val)], val, funcs...);
		}
	};
	template<typename In, typename KeyLoader>
	struct group_by<In, KeyLoader>
	{
		using Out = decltype(std::declval<KeyLoader>()(std::declval<In>()));
		typedef typename map_type<Out, typename group_by<In>::type, std::is_fundamental<Out>::value>::type type;

		constexpr static void emplace(type &handle, In const &val, KeyLoader const &func) noexcept(true)
		{
			group_by<In>::emplace(handle[func(val)], val);
		}
	};
	template<typename In>
	struct group_by<In>
	{
		typedef std::vector<typename std::remove_reference<In>::type> type;

		constexpr static void emplace(type &vec, In const &val) noexcept(true)
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
		constexpr bool next(Lhs const &lhs, Rhs const &rhs) const noexcept(true)
		{
			return lhs == rhs;
		}
	};

	template<>
	struct TFilter<eOrderType::asc> : FilterBase
	{
		template <typename Lhs, typename Rhs>
		constexpr bool operator()(Lhs const &lhs, Rhs const &rhs) const noexcept(true)
		{
			return lhs < rhs;
		}
	};

	template<>
	struct TFilter<eOrderType::desc> : FilterBase
	{
		template <typename Lhs, typename Rhs>
		constexpr bool operator()(Lhs const &lhs, Rhs const &rhs) const noexcept(true)
		{
			return lhs > rhs;
		}
	};


	template<>
	struct TFilter<eOrderType::custom>
	{
		template <typename Lhs, typename Rhs>
		constexpr bool operator()(Lhs const &lhs, Rhs const &rhs) const noexcept(true)
		{
			return false;
		}

		template <typename Lhs, typename Rhs>
		constexpr bool next(Lhs const &lhs, Rhs const &rhs) const noexcept(true)
		{
			return false;
		}
	};

	template <typename BaseFilter, typename Key, typename ...Params>
	struct Filter : BaseFilter
	{
		explicit Filter(Key const &key, Params... params) : BaseFilter(params...), key_(key) {}

		template <typename Lhs, typename Rhs>
		constexpr bool apply(Lhs const &lhs, Rhs const &rhs) const
		{
			return (static_cast<BaseFilter const &>(*this))(key_(lhs), key_(rhs));
		}

		template <typename Lhs, typename Rhs>
		constexpr bool next(Lhs const &lhs, Rhs const &rhs) const
		{
			return (static_cast<BaseFilter const &>(*this)).next(key_(lhs), key_(rhs));
		}

	private:
		Key const &key_;
	};

	template<typename In, typename Filter, typename... Filters>
	constexpr  bool order_by_current(In const &a, In const &b, Filter const &key, Filters const &...keys) noexcept(true)
	{
		return key.apply(a, b) || (key.next(a, b) && order_by_current(a, b, keys...));
	}
	template<typename In, typename Key>
	constexpr bool order_by_current(In const &a, In const &b, Key const &key) noexcept(true)
	{
		return key.apply(a, b);
	}

	template <typename Key>
	using asc_t = typename Filter<TFilter<eOrderType::asc>, Key>;
	template <typename Key>
	using desc_t = typename Filter<TFilter<eOrderType::desc>, Key>;

	template <typename BaseFilter, typename Key, typename ...Params>
	auto make_filter(Key const &key, Params... params) noexcept(true)
	{
		return Filter<BaseFilter, Key, Params...>(key, params...);
	}
	template <typename Key>
	auto asc(Key const &key) noexcept(true) { return asc_t<Key>(key); }
	template <typename Key>
	auto desc(Key const &key) noexcept(true) { return desc_t<Key>(key); }

	/*! utils */
}

#endif // !UTILITY_H_
