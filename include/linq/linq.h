#ifndef LINQ_H_INCLUDED
#define LINQ_H_INCLUDED

#include <iostream>
#include <memory>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>

//version without chevre
//version only filter

namespace linq
{
	/* utils */
	enum class iterator_type
	{
		basic,
		reach,
		filter,
		load,
		full
	};

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

	template<typename In>
	struct group_by_f
	{
		typedef std::vector<typename std::remove_reference<In>::type> type;

		inline static void emplace(type &vec, In const &val) noexcept
		{
			vec.push_back(val);
		}
	};
	template<typename In, typename KeyLoader, typename... Funcs>
	struct group_by
	{
		using Out = decltype(std::declval<KeyLoader>()(std::declval<In>()));
		typedef typename map_type<Out, typename group_by<In, Funcs...>::type, std::is_fundamental<Out>::value>::type type;

		inline static void emplace(type &handle, In const &val, KeyLoader const &func, Funcs const &...funcs) noexcept
		{
			group_by<In, Funcs...>::emplace(handle[func(val)], val, funcs...);
		}
	};
	template<typename In, typename KeyLoader>
	struct group_by<In, KeyLoader>
	{
		using Out = decltype(std::declval<KeyLoader>()(std::declval<In>()));
		typedef typename map_type<Out, typename group_by_f<In>::type, std::is_fundamental<Out>::value>::type type;

		inline static void emplace(type &handle, In const &val, KeyLoader const &func) noexcept
		{
			group_by_f<In>::emplace(handle[func(val)], val);
		}
	};
	/*! utils */

	/* iterator */
	template<typename Proxy, typename Base, typename Out, iterator_type ItType>
	class iterator;

	template<typename Proxy, typename Base, typename Out>
	class iterator<Proxy, Base, Out, iterator_type::full> : public Base
	{
		Proxy const &proxy_;

	public:
		iterator() = delete;
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(Base const &base, Proxy const &proxy)
			: Base(base), proxy_(proxy)
		{}

		iterator const &operator++() noexcept
		{
			do
			{
				static_cast<Base &>(*this).operator++();
			} while (proxy_.validate(static_cast<Base const &>(*this)));
			return (*this);
		}

		inline Out operator*() const noexcept
		{
			return proxy_.load(static_cast<Base const &>(*this));
		}

		iterator const &operator=(iterator const &rhs)
		{
			static_cast<Base &>(*this) = static_cast<Base const &>(rhs);
			return (*this);
		}

		inline bool operator!=(iterator const &rhs) const noexcept {
			return static_cast<Base const &>(*this) != static_cast<Base const &>(rhs);
		}

	};

	template<typename Proxy, typename Base, typename Out>
	class iterator<Proxy, Base, Out, iterator_type::load> : public Base
	{
		Proxy const &proxy_;
	public:
		iterator() = delete;
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(Base const &base, Proxy const &proxy)
			: Base(base), proxy_(proxy)
		{}

		inline Out operator*() const noexcept { return proxy_.load(static_cast<Base const &>(*this)); }

		inline iterator const &operator++() noexcept {
			static_cast<Base &>(*this).operator++();
			return *this;
		}

		iterator const &operator=(iterator const &rhs)
		{
			static_cast<Base &>(*this) = static_cast<Base const &>(rhs);
			return (*this);
		}

		inline bool operator!=(iterator const &rhs) const noexcept {
			return static_cast<Base const &>(*this) != static_cast<Base const &>(rhs);
		}
	};

	template<typename Proxy, typename Base, typename Out>
	class iterator<Proxy, Base, Out, iterator_type::filter> : public Base
	{
		Proxy const &proxy_;
	public:
		iterator() = delete;
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(Base const &base, Proxy const &proxy)
			: Base(base), proxy_(proxy)
		{}

		inline Out operator*() const noexcept { return *static_cast<Base const &>(*this); }

		iterator const &operator++() noexcept {
			do
			{
				static_cast<Base &>(*this).operator++();
			} while (proxy_.validate(static_cast<Base const &>(*this)));
			return (*this);
		}

		iterator const &operator=(iterator const &rhs)
		{
			static_cast<Base &>(*this) = static_cast<Base const &>(rhs);
			return (*this);
		}

		inline bool operator!=(iterator const &rhs) const noexcept {
			return static_cast<Base const &>(*this) != static_cast<Base const &>(rhs);
		}
	};

	template<typename Proxy, typename Base, typename Out>
	class iterator<Proxy, Base, Out, iterator_type::reach> : public Base
	{
		Proxy const &proxy_;
	public:
		iterator() = delete;
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(Base const &base, Proxy const &proxy)
			: Base(base), proxy_(proxy)
		{}

		inline Out operator*() const noexcept { return *static_cast<Base const &>(*this); }

		iterator const &operator=(iterator const &rhs)
		{
			static_cast<Base &>(*this) = static_cast<Base const &>(rhs);
			return (*this);
		}

		inline bool operator!=(iterator const &rhs) const noexcept {
			return  static_cast<Base const &>(*this) != static_cast<Base const &>(rhs) && proxy_.reach(static_cast<Base const &>(*this));
		}

		iterator const &operator++() noexcept {
			proxy_.incr(static_cast<Base const &>(*this));
			static_cast<Base &>(*this).operator++();
			return (*this);
		}

	};

	template<typename Proxy, typename Base, typename Out>
	class iterator<Proxy, Base, Out, iterator_type::basic> : public Base
	{
	public:
		iterator() = delete;
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(Base const &base, Proxy const &)
			: Base(base)
		{}

		iterator const &operator=(iterator const &rhs)
		{
			static_cast<Base &>(*this) = static_cast<Base const &>(rhs);
			return (*this);
		}
	};

	/*Linq statements*/
	template<typename Proxy, typename Base_It, typename Out, iterator_type ItType>
	class base;
	//GroupBy
	template<typename Iterator, typename Proxy>
	class GroupBy : public base
		<
		GroupBy<Iterator, Proxy>,
		Iterator,
		decltype(*std::declval<Iterator>()),
		iterator_type::basic
		>
	{

	public:
		using proxy_t = GroupBy<Iterator, Proxy>;
		using Out = decltype(*std::declval<Iterator>());
		using base_t = base<proxy_t, Iterator, Out, iterator_type::basic>;

		using iterator_t = linq::iterator<proxy_t, Iterator, Out, iterator_type::basic>;

		typedef iterator_t iterator;
		typedef iterator_t const_iterator;

	private:
		Proxy proxy_;
	public:
		using last_map_t = decltype(*(proxy_->begin()));
		using last_key_t = decltype(std::declval<last_map_t>().first);

		GroupBy() = delete;
		GroupBy(GroupBy const &) = default;
		GroupBy(Iterator const &begin, Iterator const &end, Proxy proxy)
			: base_t(begin, end), proxy_(proxy)
		{}

		//template<typename Func, bool is_basic = std::is_fundamental<decltype(std::declval<Func>()(std::declval<Out>()))>::value>
		//auto thenBy(Func const &key) const noexcept
		//{
		//	using last_key_t = decltype(std::declval<last_map_t>().first);
		//	using last_vector_t = decltype(std::declval<last_map_t>().second);
		//	using last_value_t = decltype(*std::declval<last_vector_t>().begin());

		//	using MapOut = std::map<last_key_t, std::map<decltype(key(std::declval<last_value_t>())), std::vector<typename std::remove_reference<last_value_t>::type>>>;

		//	auto result = std::make_shared<MapOut>();

		//	for (Out it : *proxy_)
		//	{
		//		for (last_value_t val : it.second)
		//			(*result)[it.first][key(val)].push_back(val);
		//	}

		//	return GroupBy<typename MapOut::iterator, decltype(result)>(result->begin(), result->end(), result);
		//}

		inline auto &operator[](last_key_t const &key) const
		{
			return (*proxy_)[key];
		}

	};
	//SelectWhere
	template<typename Iterator, typename Filter, typename Loader>
	class SelectWhere
		: public base
		<
		SelectWhere<Iterator, Filter, Loader>,
		Iterator,
		decltype(std::declval<Loader>()(std::declval<decltype(*std::declval<Iterator>())>())),
		iterator_type::full
		>
	{
	public:
		using base_iterator_t = Iterator;
		using value_type = decltype(*std::declval<Iterator>());
		using Out = decltype(std::declval<Loader>()(std::declval<value_type>()));

		using proxy_t = SelectWhere<Iterator, Filter, Loader>;
		using base_t = base<proxy_t, base_iterator_t, Out, iterator_type::full>;
		using iterator_t = linq::iterator<proxy_t, base_iterator_t, Out, iterator_type::full>;

		typedef iterator_t iterator;
		typedef iterator_t const_iterator;

	private:
		friend iterator_t;
		Filter const _filter;
		Loader const _loader;

		inline bool validate(base_iterator_t const &it) const noexcept
		{
			return
				it != static_cast<base_iterator_t const &>(this->_end)
				&& !_filter(*it);
		}

		inline Out load(base_iterator_t const &it) const noexcept
		{
			return _loader(*it);
		}

	public:
		SelectWhere() = delete;
		~SelectWhere() = default;
		SelectWhere(SelectWhere const &) = default;
		SelectWhere(base_iterator_t const &begin, base_iterator_t const &end,
			Filter const &filter,
			Loader const &loader)
			: base_t(begin, end), _filter(filter), _loader(loader)
		{}

		template<typename Func>
		auto select(Func const &next_loader) const noexcept
		{
			const auto &last_loader = _loader;
			const auto new_loader = [last_loader, next_loader](value_type value) noexcept -> decltype(next_loader(std::declval<Out>()))
			{
				return next_loader(last_loader(value));
			};
			return SelectWhere<Iterator, Filter, decltype(new_loader)>(
				static_cast<base_iterator_t const &>(this->_begin),
				static_cast<base_iterator_t const &>(this->_end),
				_filter,
				new_loader);
		}

		template<typename Func>
		auto where(Func const &next_filter) const noexcept
		{
			const auto &last_loader = _loader;
			const auto &last_filter = _filter;
			const auto &new_filter = [last_loader, last_filter, next_filter](value_type value) noexcept -> bool
			{
				return last_filter(value) && next_filter(last_loader(value));
			};
			return SelectWhere<Iterator, decltype(new_filter), Loader>(
				std::find_if(static_cast<base_iterator_t const &>(this->_begin), static_cast<base_iterator_t const &>(this->_end), new_filter),
				static_cast<base_iterator_t const &>(this->_end),
				new_filter,
				last_loader);
		}

	};
	//Select
	template<typename Iterator, typename Loader>
	class Select
		: public base
		<
		Select<Iterator, Loader>,
		Iterator,
		decltype(std::declval<Loader>()(std::declval<decltype(*std::declval<Iterator>())>())),
		iterator_type::load>
	{
	public:
		using base_iterator_t = Iterator;
		using value_type = decltype(*std::declval<Iterator>());
		using Out = decltype(std::declval<Loader>()(std::declval<value_type>()));

		using proxy_t = Select<Iterator, Loader>;
		using base_t = base<proxy_t, base_iterator_t, Out, iterator_type::load>;
		using iterator_t = linq::iterator<proxy_t, base_iterator_t, Out, iterator_type::load>;

		typedef iterator_t iterator;
		typedef iterator_t const_iterator;
	private:
		friend iterator_t;
		Loader const _loader;

		inline Out load(base_iterator_t const &it) const noexcept
		{
			return _loader(*it);
		}
	public:
		Select() = delete;
		~Select() = default;
		Select(Select const &) = default;
		Select(base_iterator_t const &begin, base_iterator_t const &end,
			Loader const &loader)
			: base_t(begin, end), _loader(loader)
		{}

		template<typename Func>
		auto select(Func const &next_loader) const noexcept
		{
			const auto &last_loader = _loader;
			const auto &new_loader = [last_loader, next_loader](value_type value) noexcept -> decltype(next_loader(std::declval<Out>()))
			{
				return next_loader(last_loader(value));
			};
			return Select<Iterator, decltype(new_loader)>(
				static_cast<base_iterator_t const &>(this->_begin),
				static_cast<base_iterator_t const &>(this->_end),
				new_loader);
		}

		template<typename Func>
		auto where(Func const &next_filter) const noexcept
		{
			const auto &last_loader = _loader;
			const auto &new_filter = [last_loader, next_filter](value_type value) noexcept -> bool
			{
				return next_filter(last_loader(value));
			};
			return SelectWhere<Iterator, decltype(new_filter), Loader>(
				std::find_if(static_cast<base_iterator_t const &>(this->_begin), static_cast<base_iterator_t const &>(this->_end), new_filter),
				static_cast<base_iterator_t const &>(this->_end),
				new_filter,
				last_loader);
		}
	};
	//Where
	template<typename Iterator, typename Filter>
	class Where
		: public base
		<
		Where<Iterator, Filter>,
		Iterator,
		decltype(*std::declval<Iterator>()),
		iterator_type::filter
		>
	{
	public:
		using base_iterator_t = Iterator;
		using value_type = decltype(*std::declval<Iterator>());
		using Out = decltype(*std::declval<Iterator>());

		using proxy_t = Where<Iterator, Filter>;
		using base_t = base<proxy_t, base_iterator_t, Out, iterator_type::filter>;
		using iterator_t = linq::iterator<proxy_t, base_iterator_t, Out, iterator_type::filter>;

		typedef iterator_t iterator;
		typedef iterator_t const_iterator;

	private:
		friend iterator_t;
		Filter const _filter;

		inline bool validate(base_iterator_t const &it) const noexcept
		{
			return
				it != static_cast<base_iterator_t const &>(this->_end)
				&& !_filter(*it);
		}

	public:
		Where() = delete;
		~Where() = default;
		Where(Where const &) = default;
		Where(base_iterator_t const &begin, base_iterator_t const &end, Filter const &filter)
			: base_t(begin, end), _filter(filter)
		{}

		template<typename Func>
		auto select(Func const &next_loader) const noexcept
		{
			return SelectWhere<Iterator, Filter, decltype(next_loader)>(
				static_cast<base_iterator_t const &>(this->_begin),
				static_cast<base_iterator_t const &>(this->_end),
				_filter,
				next_loader);
		}

		template<typename Func>
		auto where(Func const &next_filter) const noexcept
		{
			const auto &last_filter = _filter;
			const auto &new_filter = [last_filter, next_filter](value_type value) noexcept -> bool
			{
				return last_filter(value) && next_filter(value);
			};
			return Where<Iterator, Filter>(
				std::find_if(static_cast<base_iterator_t const &>(this->_begin), static_cast<base_iterator_t const &>(this->_end), new_filter),
				static_cast<base_iterator_t const &>(this->_end),
				new_filter);
		}
	};
	//Take
	template <typename Iterator>
	class Take : public base
		<
		Take<Iterator>,
		Iterator,
		decltype(*std::declval<Iterator>()),
		iterator_type::reach
		>
	{
	public:
		using Out = decltype(*std::declval<Iterator>());

		using proxy_t = Take<Iterator>;
		using base_t = base
			<
			Take<Iterator>,
			Iterator,
			Out,
			iterator_type::reach
			>;
		using iterator_t = linq::iterator<proxy_t, Iterator, Out, iterator_type::reach>;

		typedef iterator_t iterator;
		typedef iterator_t const_iterator;
	private:
		friend iterator_t;

		mutable std::size_t number_;
		inline bool reach(Iterator const &) const { return number_ > 0; }
		inline void incr(Iterator const &) const { --number_; }

	public:
		Take() = delete;
		~Take() = default;
		Take(Take const &) = default;

		Take(Iterator const &begin, Iterator const &end, std::size_t number)
			: base_t(begin, end), number_(number)
		{}

		auto take(std::size_t number) const
		{
			return Take<Iterator>(this->_begin, this->_end, number);
		}
	};
	//From
	template<typename Iterator>
	class From
		: public base
		<
		From<Iterator>,
		Iterator,
		decltype(*std::declval<Iterator>()),
		iterator_type::basic
		>
	{
	public:
		using base_iterator_t = Iterator;
		using value_type = decltype(*std::declval<Iterator>());


		using proxy_t = From<Iterator>;
		using base_t = base<proxy_t, base_iterator_t, value_type, iterator_type::basic>;

		typedef Iterator iterator;
		typedef Iterator const_iterator;

	public:

		From() = delete;
		~From() = default;
		From(From const &) = default;
		From(base_iterator_t const &begin, base_iterator_t const &end)
			: base_t(begin, end)
		{}

	};
	//IState
	template<typename Proxy, typename Base_It, typename Out, iterator_type ItType>
	class base
	{
		using iterator_t = iterator<Proxy, Base_It, Out, ItType>;
	protected:
		iterator_t const _begin;
		iterator_t const _end;
	public:
		base() = delete;
		~base() = default;
		base(base const &rhs)
			: 
			_begin(rhs._begin, static_cast<Proxy const &>(*this)),
			_end(rhs._end, static_cast<Proxy const &>(*this))
		{}
		base(Base_It const &begin, Base_It const &end)
			:
			_begin(begin, static_cast<Proxy const &>(*this)),
			_end(end, static_cast<Proxy const &>(*this))
		{}

		inline iterator_t const begin() const noexcept { return _begin; }
		inline iterator_t const end() const noexcept { return _end; }


		template<typename Func>
		auto select(Func const &next_loader) const noexcept
		{
			return Select<Base_It, Func>(
					static_cast<Base_It const &>(this->_begin),
					static_cast<Base_It const &>(this->_end),
					next_loader);
		}
		template<typename Func>
		auto where(Func const &next_filter) const noexcept
		{
			return Where<Base_It, Func>(
					std::find_if(static_cast<Base_It const &>(this->_begin), static_cast<Base_It const &>(this->_end), next_filter),
					static_cast<Base_It const &>(this->_end),
					next_filter);
		}
		
		template<typename Func>
		auto skipWhile(Func const &func) const noexcept
		{
			auto ret = _begin;
			while(static_cast<Base_It const &>(ret) != static_cast<Base_It const &>(_end) && func(*ret))
				++ret;
			return From<iterator_t>(ret, _end);
		}
		auto skip(std::size_t offset) const noexcept
		{
			auto ret = _begin;
			for (std::size_t i = 0; i < offset && ret != _end; ++ret, ++i);

			return From<iterator_t>(ret, _end);
		}

		// todo takeWhile
		auto take(std::size_t number) const
		{
			return Take<iterator_t>(_begin, _end, number);
		}

		//template<typename typename... Funcs>
		//auto orderBy(Funcs... keys) const noexcept
		//{
		//	auto comp = [keys](Out a, Out b) -> bool
		//	{
		//		std:: keys(a, b)
		//	}

		//	using group_type = group_by<Out, Funcs...>;
		//	using map_out = typename group_type::type;
		//	auto result = std::make_shared<map_out>();

		//	for (Out it : *this)
		//		group_type::emplace(*result, it, keys...);

		//	return GroupBy<typename map_out::iterator, decltype(result)>(result->begin(), result->end(), result);
		//}

		template<typename... Funcs>
		auto groupBy(Funcs... keys) const noexcept
		{
			using group_type = group_by<Out, Funcs...>;
			using map_out = typename group_type::type;
			auto result = std::make_shared<map_out>();

			for (Out it : *this)
				group_type::emplace(*result, it, keys...);

			return GroupBy<typename map_out::iterator, decltype(result)>(result->begin(), result->end(), result);
		}

		auto min() const noexcept
		{
			typename std::remove_const<typename std::remove_reference<decltype(*_begin)>::type>::type val(*_begin);
			for (Out it : *this)
				if (it > val)
					val = it;
			return val;
		}
		auto max() const noexcept
		{
			typename std::remove_const<typename std::remove_reference<decltype(*_begin)>::type>::type val(*_begin);
			for (Out it : *this)
				if (it < val)
					val = it;
			return val;
		}
		auto sum() const noexcept
		{
			typename std::remove_const<typename std::remove_reference<decltype(*_begin)>::type>::type result{};
			for (Out it : *this)
				result += it;
			return result;
		}
		auto count() const noexcept
		{
			std::size_t number{ 0 };
			for (auto it = _begin; it != _end; ++it, ++number);
			return number;
		}

		template<typename OutContainer>
		OutContainer to() const noexcept
		{
			OutContainer out;
			std::copy(_begin, _end, std::back_inserter(out));
			return std::move(out);
		}
		template<typename OutContainer>
		inline void to(OutContainer &out) const noexcept
		{
			std::copy(_begin, _end, std::back_inserter(out));
		}

	};

	template <typename Handle>
	class IEnumerable : public Handle
	{
		typedef typename Handle::iterator iterator_t;
		typedef decltype(*std::declval<iterator_t>()) out_t;

	public:
		IEnumerable() = delete;
		~IEnumerable() = default;
		IEnumerable(IEnumerable const &) = default;
		IEnumerable(Handle const &rhs)
			: Handle(rhs) {}
		
		template<typename Func>
		inline auto Select(Func const &next_loader) const noexcept
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).select(next_loader))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).select(next_loader)));
		}
		template<typename Func>
		inline auto Where(Func const &next_filter) const noexcept
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).where(next_filter))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).where(next_filter)));
		}
		
		template<typename Func>
		inline auto SkipWhile(Func const &func) const noexcept
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).skipWhile(func))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).skipWhile(func)));
		}		
		// TakeWhile todo
		
		inline auto Skip(std::size_t offset) const noexcept
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).skip(offset))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).skip(offset)));
		}		
		inline auto Take(std::size_t limit) const noexcept
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).take(limit))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).take(limit)));
		}
		
		template<typename... Funcs>
		inline auto GroupBy(Funcs... keys) const noexcept
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).groupBy(keys...))>;
			return ret_t(static_cast<Handle const &>(*this).groupBy(keys...));
		}

		template<typename Func>
		inline auto ThenBy(Func const &key) const noexcept
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).thenBy(key))>;
			return ret_t(static_cast<Handle const &>(*this).thenBy(key));
		}

		inline auto Min() const noexcept
		{
			return std::move(static_cast<Handle const &>(*this).min());
		}
		inline auto Max() const noexcept
		{
			return std::move(static_cast<Handle const &>(*this).max());
		}
		inline auto Sum() const noexcept
		{
			return std::move(static_cast<Handle const &>(*this).sum());
		}
		inline auto Count() const noexcept
		{
			return static_cast<Handle const &>(*this).count();
		}
		inline const iterator_t begin() const noexcept
		{
			return std::move(static_cast<Handle const &>(*this).begin());
		}
		inline const iterator_t end() const noexcept
		{
			return std::move(static_cast<Handle const &>(*this).end());
		}

		//template<typename OutProxy>
		//inline OutProxy To() const noexcept
		//{
		//	return std::move(static_cast<Handle &>(*this).to< OutProxy >());
		//}
		template<typename OutProxy>
		inline void To(OutProxy &out) const noexcept
		{
			return std::move(static_cast<Handle const &>(*this).to(out));
		}

		template<typename T>
		inline auto &operator[](T const &key) const
		{
			return static_cast<Handle const &>(*this).operator[](key);
		}

	};

	template<typename T>
	auto make_enumerable(T const &container)
	{
		return IEnumerable<From<typename T::const_iterator>>(From<typename T::const_iterator>(std::begin(container), std::end(container)));
	}
	template<typename T>
	auto make_enumerable(T &container)
	{
		return IEnumerable<From<typename T::iterator>>(From<typename T::iterator>(std::begin(container), std::end(container)));
	}
	template<typename T>
	auto make_enumerable(T const &begin, T const &end)
	{
		return IEnumerable<From<T>>(From<T>(begin, end));
	}

	template<typename T>
	auto from(T const &container)
	{
		return std::move(linq::From<typename T::const_iterator>(std::begin(container), std::end(container)));
	}

	template<typename T>
	auto from(T &container)
	{
		return std::move(linq::From<typename T::iterator>(std::begin(container), std::end(container)));
	}

	template<typename T>
	auto range(T const &begin, T const &end)
	{
		return std::move(linq::From<T>(begin, end));
	}

}

#endif // LINQ_H_INCLUDED
