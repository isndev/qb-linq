#ifndef LINQ_H_INCLUDED
#define LINQ_H_INCLUDED

#include <iostream>
#include <memory>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>
#include <map>
#include <algorithm>

//version without chevre
//version only filter

namespace linq
{
	enum class iterator_type
	{
		basic,
		reach,
		filter,
		load,
		full
	};

	template<typename Proxy, typename Base, typename Out = void, iterator_type ItType = iterator_type::basic>
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
			auto end = static_cast<Base const &>(proxy_._end);
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
			return  static_cast<Base const &>(*this) != static_cast<Base const &>(rhs) && proxy_.reach();
		}

		iterator const &operator++() noexcept {
			--proxy_.number_;
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

	//template<typename Proxy, typename Base, typename Out>
	//class limit : public iterator

	/*Linq Object with Filter and Loader*/
	template<typename Proxy, typename Base_It, typename Out, iterator_type ItType>
	class base;

	template<typename Iterator, typename Filter = void, typename Loader = void, iterator_type ItType = iterator_type::basic>
	class Object;

	template<typename Iterator, typename Proxy>
	class GroupBy;

	template <typename Iterator>
	class Take;

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
		base(base const &) = default;
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
			return Object<Base_It, void, Func, iterator_type::load>(
				static_cast<Base_It const &>(this->_begin),
				static_cast<Base_It const &>(this->_end),
				next_loader);
		}
		template<typename Func>
		auto where(Func const &next_filter) const noexcept
		{
			return Object<Base_It, Func, void, iterator_type::filter>(
				std::find_if(static_cast<Base_It const &>(this->_begin), static_cast<Base_It const &>(this->_end), next_filter),
				static_cast<Base_It const &>(this->_end),
				next_filter);
		}
		template<typename Func>
		auto skip(Func const &func, std::size_t offset) const noexcept
		{
			auto ret = _begin;
			for (std::size_t i = 0;
				i < offset
				&& static_cast<Base_It const &>(ret) != static_cast<Base_It const &>(_end)
				&& func(*ret); ++ret, ++i);

			return Object<iterator_t>(ret, _end);
		}
		auto skip(std::size_t offset) const noexcept
		{
			auto ret = _begin;
			for (std::size_t i = 0; i < offset && ret != _end; ++ret, ++i);

			return Object<iterator_t>(ret, _end);
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

		auto take(int number) const
		{
			return Take<iterator_t>(_begin, _end, number);
		}

		template<typename Func>
		auto groupBy(Func const &key) const noexcept
		{
			using MapOut = std::map<decltype(key(std::declval<Out>())), std::vector<Out>>;

			auto result = std::make_shared<MapOut>();

			for (Out it : *this)
				(*result)[key(it)].push_back(it);

			return GroupBy<typename MapOut::iterator, decltype(result)>(result->begin(), result->end(), result);
		}

	};

	template<typename Iterator, typename Proxy>
	class GroupBy : public Object<Iterator>
	{
		Proxy proxy_;
	public:
		GroupBy() = delete;
		GroupBy(GroupBy const &) = default;
		GroupBy(Iterator const &begin, Iterator const &end, Proxy proxy)
			: Object<Iterator>(begin, end), proxy_(proxy)
		{}

	};

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

		mutable int number_;
		inline bool reach() const { return number_ > 0; }

	public:
		Take() = delete;
		~Take() = default;
		Take(Take const &) = default;

		Take(Iterator const &begin, Iterator const &end, int number)
			: base_t(begin, end), number_(number)
		{}

		auto take(int number) const
		{
			return Take<Iterator>(this->_begin, this->_end, number);
		}
	};

	// skip = ?select+?where+ & new begin, end
	// take = ?select+?where+ & begin, end - limit 
	// skipWhile = ?select+?where+ & new begin(T), end
	// takeWhile = ?select+?where+ & begin, end - limit

	//SelectWhere
	template<typename Iterator, typename Filter, typename Loader>
	class Object<Iterator, Filter, Loader, iterator_type::full>
		: public base
		<
		Object<Iterator, Filter, Loader, iterator_type::full>,
		Iterator,
		decltype(std::declval<Loader>()(std::declval<decltype(*std::declval<Iterator>())>())),
		iterator_type::full
		>
	{
	public:
		using base_iterator_t = Iterator;
		using value_type = decltype(*std::declval<Iterator>());
		using Out = decltype(std::declval<Loader>()(std::declval<value_type>()));

		using proxy_t = Object<Iterator, Filter, Loader, iterator_type::full>;
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
		Object() = delete;
		~Object() = default;
		Object(Object const &) = default;
		Object(base_iterator_t const &begin, base_iterator_t const &end,
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
			return Object<Iterator, Filter, decltype(new_loader), iterator_type::full>(
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
			return Object<Iterator, decltype(new_filter), Loader, iterator_type::full>(
				std::find_if(static_cast<base_iterator_t const &>(this->_begin), static_cast<base_iterator_t const &>(this->_end), new_filter),
				static_cast<base_iterator_t const &>(this->_end),
				new_filter,
				last_loader);
		}

	};
	//Select
	template<typename Iterator, typename Loader>
	class Object<Iterator, void, Loader, iterator_type::load>
		: public base
		<
		Object<Iterator, void, Loader, iterator_type::load>,
		Iterator,
		decltype(std::declval<Loader>()(std::declval<decltype(*std::declval<Iterator>())>())),
		iterator_type::load>
	{
	public:
		using base_iterator_t = Iterator;
		using value_type = decltype(*std::declval<Iterator>());
		using Out = decltype(std::declval<Loader>()(std::declval<value_type>()));

		using proxy_t = Object<Iterator, void, Loader, iterator_type::load>;
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
		Object() = delete;
		~Object() = default;
		Object(Object const &) = default;
		Object(base_iterator_t const &begin, base_iterator_t const &end,
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
			return Object<Iterator, void, decltype(new_loader), iterator_type::load>(
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
			return Object<Iterator, decltype(new_filter), Loader, iterator_type::full>(
				std::find_if(static_cast<base_iterator_t const &>(this->_begin), static_cast<base_iterator_t const &>(this->_end), new_filter),
				static_cast<base_iterator_t const &>(this->_end),
				new_filter,
				last_loader);
		}
	};
	//Where
	template<typename Iterator, typename Filter>
	class Object<Iterator, Filter, void, iterator_type::filter> 
		: public base
		<
		Object<Iterator, Filter, void, iterator_type::filter>,
		Iterator,
		decltype(*std::declval<Iterator>()),
		iterator_type::filter
		>
	{
	public:
		using base_iterator_t = Iterator;
		using value_type = decltype(*std::declval<Iterator>());
		using Out = decltype(*std::declval<Iterator>());

		using proxy_t = Object<Iterator, Filter, void, iterator_type::filter>;
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
		Object() = delete;
		~Object() = default;
		Object(Object const &) = default;
		Object(base_iterator_t const &begin, base_iterator_t const &end, Filter const &filter)
			: base_t(begin, end), _filter(filter)
		{}

		template<typename Func>
		auto select(Func const &next_loader) const noexcept
		{
			return Object<Iterator, Filter, decltype(next_loader), iterator_type::full>(
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
			return Object<Iterator, Filter, void, iterator_type::filter>(
				std::find_if(static_cast<base_iterator_t const &>(this->_begin), static_cast<base_iterator_t const &>(this->_end), new_filter),
				static_cast<base_iterator_t const &>(this->_end),
				new_filter);
		}
	};
	//From
	template<typename Iterator>
	class Object<Iterator, void, void, iterator_type::basic>
		: public base
		<
		Object<Iterator, void, void, iterator_type::basic>,
		Iterator,
		decltype(*std::declval<Iterator>()),
		iterator_type::basic
		>
	{
	public:
		using base_iterator_t = Iterator;
		using value_type = decltype(*std::declval<Iterator>());


		using proxy_t = Object<Iterator, void, void, iterator_type::basic>;
		using base_t = base<proxy_t, base_iterator_t, value_type, iterator_type::basic>;

		typedef Iterator iterator;
		typedef Iterator const_iterator;

	public:

		Object() = delete;
		~Object() = default;
		Object(Object const &) = default;
		Object(base_iterator_t const &begin, base_iterator_t const &end)
			: base_t(begin, end)
		{}

	};






	template<typename T>
	auto from(T const &container)
	{
		return std::move(linq::Object<typename T::const_iterator>(std::begin(container), std::end(container)));
	}

	template<typename T>
	auto from(T &container)
	{
		return std::move(linq::Object<typename T::iterator>(std::begin(container), std::end(container)));
	}

	template<typename T>
	auto range(T const &begin, T const &end)
	{
		return std::move(linq::Object<T>(begin, end));
	}

}

#endif // LINQ_H_INCLUDED
