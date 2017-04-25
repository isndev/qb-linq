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

	enum class order_type
	{
		asc,
		desc
	};
	template<typename Key, order_type OrderType = order_type::asc>
	struct order_by_modifier
	{
		typedef Key key_type;
		static constexpr order_type ob_type = OrderType;

		Key const &key_;
		order_by_modifier(Key const &key) : key_(key) {}
	};

	template <typename Key>
	auto const asc(Key const &key) noexcept(true) { return std::move(order_by_modifier<Key, order_type::asc>(key)); }
	template <typename Key>
	auto const desc(Key const &key) noexcept(true) { return std::move(order_by_modifier<Key, order_type::desc>(key)); }

	template<typename In, typename Base>
	struct order_by_modifier_end : public Base
	{
		static constexpr order_type type = Base::ob_type;

		order_by_modifier_end() = delete;
		~order_by_modifier_end() = default;
		order_by_modifier_end(order_by_modifier_end const &) = default;
		order_by_modifier_end(Base const &key) : Base(key.key_) {}
		order_by_modifier_end(typename Base::key_type const &key) : Base(key) {}

		auto operator()(In val) const noexcept(true) { return this->key_(val); }

	};


	template<typename In, order_type OrderType>
	struct order_by_less
	{
		constexpr bool operator()(const In a, const In b) const noexcept(true)
		{
			return a < b;
		}
	};
	template<typename In>
	struct order_by_less<In, order_type::desc>
	{
		constexpr bool operator()(const In a, const In b) const  noexcept(true)
		{
			return a < b;
		}
	};
	template<typename In, typename Key1, typename Key2, typename... Keys>
	constexpr bool order_by_next(In &a, In &b, Key1 const &key1, Key2 const &key2, Keys const &...keys) noexcept(true)
	{
		return key1(a) == key1(b) && (order_by_less<decltype(key2(a)), Key2::type>()(key2(a), key2(b)) || order_by_next(a, b, key2, keys...));
	}
	template<typename In, typename Key1, typename Key2>
	constexpr bool order_by_next(In &a, In &b, Key1 const &key1, Key2 const &key2) noexcept(true)
	{
		return key1(a) == key1(b) && order_by_less<decltype(key2(a)), Key2::type>()(key2(a), key2(b));
	}
	template<typename In, typename Key, typename... Keys>
	constexpr bool order_by(In &a, In &b, Key const &key, Keys const &...keys) noexcept(true)
	{
		return order_by_less<decltype(key(a)), Key::type>()(key(a), key(b)) || order_by_next(a, b, key, keys...);
	}
	template<typename In, typename Key>
	constexpr bool order_by(In &a, In &b, Key const &key) noexcept(true)
	{
		return order_by_less<decltype(key(a)), Key::type>()(key(a), key(b));
	}
	/*! utils */
}

#endif // !UTILITY_H_
