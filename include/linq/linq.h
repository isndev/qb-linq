#pragma once

#include <iostream>
#include <memory>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>
#include <map>
#include <algorithm>

//version withouth
//version only filter

namespace linq
{

	template<typename _Handle, typename _Base, typename _Out = void, int _Level = 1>
	class iterator : public _Base
	{
		_Handle const &_handle;
	public:
		iterator() = delete;
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(_Base const &base, _Handle const &handle)
			: _Base(base), _handle(handle)
		{}

		inline _Out operator*() const noexcept { return _handle._loader(*static_cast<_Base const &>(*this)); }

		iterator const &operator++() noexcept {
			do
			{
				static_cast<_Base &>(*this).operator++();
			} while (static_cast<_Base const &>(*this) != static_cast<_Base const &>(_handle._end) && !_handle._filter(*static_cast<_Base const &>(*this)));
			return (*this);
		}

		iterator const &operator=(iterator const &rhs)
		{
			static_cast<_Base &>(*this) = static_cast<_Base const &>(rhs);
			return (*this);
		}

		inline bool operator!=(iterator const &rhs) {
			return static_cast<_Base const &>(*this) != static_cast<_Base const &>(rhs);
		}

	};

	template<typename _Handle, typename _Base, typename _Out>
	class iterator<_Handle, _Base, _Out, 3> : public _Base
	{
		_Handle const &_handle;
	public:
		iterator() = delete;
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(_Base const &base, _Handle const &handle)
			: _Base(base), _handle(handle)
		{}

		inline _Out operator*() const noexcept { return _handle._loader(*static_cast<_Base const &>(*this)); }

		inline iterator const &operator++() noexcept {
			static_cast<_Base &>(*this).operator++();
			return *this;
		}

		iterator const &operator=(iterator const &rhs)
		{
			static_cast<_Base &>(*this) = static_cast<_Base const &>(rhs);
			return (*this);
		}

		inline bool operator!=(iterator const &rhs) {
			return static_cast<_Base const &>(*this) != static_cast<_Base const &>(rhs);
		}
	};

	template<typename _Handle, typename _Base, typename _Out>
	class iterator<_Handle, _Base, _Out, 2> : public _Base
	{
		_Handle const &_handle;
	public:
		iterator() = delete;
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(_Base const &base, _Handle const &handle)
			: _Base(base), _handle(handle)
		{}

		inline _Out operator*() const noexcept { return *static_cast<_Base const &>(*this); }

		iterator const &operator++() noexcept {
			auto end = static_cast<_Base const &>(_handle._end);
			do
			{
				static_cast<_Base &>(*this).operator++();

			} while (static_cast<_Base const &>(*this) != end && !_handle._filter(*static_cast<_Base const &>(*this)));
			return (*this);
		}

		iterator const &operator=(iterator const &rhs)
		{
			static_cast<_Base &>(*this) = static_cast<_Base const &>(rhs);
			return (*this);
		}

		inline bool operator!=(iterator const &rhs) {
			return static_cast<_Base const &>(*this) != static_cast<_Base const &>(rhs);
		}
	};

	template<typename _Handle, typename _Base, typename _Out>
	class iterator<_Handle, _Base, _Out, 1> : public _Base
	{
	public:
		iterator() = delete;
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(_Base const &base, _Handle const &)
			: _Base(base)
		{}

		iterator const &operator=(iterator const &rhs)
		{
			static_cast<_Base &>(*this) = static_cast<_Base const &>(rhs);
			return (*this);
		}
	};

	/*Linq Object with Filter and Loader*/
	template<typename Iterator, typename Filter = void, typename Loader = void, int _Level = 1>
	class Object;

	template<typename Iterator, typename Handle>
	class GroupBy;

	template<typename _Handle, typename _Base_It, typename _Out, int _Level>
	class Base
	{
		using iterator_t = iterator<_Handle, _Base_It, _Out, _Level>;
	protected:
		iterator_t const _begin;
		iterator_t const _end;
	public:
		Base() = delete;
		~Base() = default;
		Base(Base const &) = default;
		Base(_Base_It const &begin, _Base_It const &end)
			:
			_begin(begin, static_cast<_Handle const &>(*this)),
			_end(end, static_cast<_Handle const &>(*this))
		{}

		inline iterator_t const begin() const noexcept { return _begin; }
		inline iterator_t const end() const noexcept { return _end; }

		auto min() const noexcept
		{
			std::remove_const<std::remove_reference<decltype(*_begin)>::type>::type val(*_begin);
			for (_Out it : *this)
				if (it > val)
					val = it;
			return val;
		}

		auto max() const noexcept
		{
			std::remove_const<std::remove_reference<decltype(*_begin)>::type>::type val(*_begin);
			for (_Out it : *this)
				if (it < val)
					val = it;
			return val;
		}

		auto sum() const noexcept
		{
			std::remove_const<std::remove_reference<decltype(*_begin)>::type>::type result{};
			for (_Out it : *this)
				result += it;
			return result;
		}

		auto count() const noexcept
		{
			std::size_t number{ 0 };
			for (auto it = _begin; it != _end; ++it, ++number);
			return number;
		}

		auto take(std::size_t offset) const noexcept
		{
			auto ret = _begin;
			for (std::size_t i = 0; i < offset && ret != _end; ++ret, ++i);

			return Object<iterator_t>(_begin, ret);
		}

		auto skip(std::size_t offset) const noexcept
		{
			auto ret = _begin;
			for (std::size_t i = 0; i < offset && ret != _end; ++ret, ++i);

			return Object<iterator_t>(ret, _end);
		}

		template<typename Func>
		auto groupBy(Func const &key) const noexcept
		{
			using MapOut = std::map<decltype(key(std::declval<_Out>())), std::vector<_Out>>;

			auto result = std::make_shared<MapOut>();

			for (_Out it : *this)
				(*result)[key(it)].push_back(it);

			return GroupBy<typename MapOut::iterator, decltype(result)>(result->begin(), result->end(), result);
		}

		template<typename _OutContainer>
		_OutContainer to() const noexcept
		{
			_OutContainer out;
			std::copy(_begin, _end, std::back_inserter(out));
			return std::move(out);
		}

		template<typename _OutContainer>
		inline void to(_OutContainer &out) const noexcept
		{
			std::copy(_begin, _end, std::back_inserter(out));
		}


	};

	template<typename Iterator, typename Handle>
	class GroupBy : public Object<Iterator>
	{
		Handle _handle;
		public:
			GroupBy() = delete;
			GroupBy(GroupBy const &) = default;
			GroupBy(Iterator const &begin, Iterator const &end, Handle handle)
				: Object<Iterator>(begin, end), _handle(handle)
			{}

	};

	template<typename Iterator, typename Filter, typename Loader>
	class Object<Iterator, Filter, Loader, 4> : public Base<Object<Iterator, Filter, Loader, 4>, Iterator,
		decltype(std::declval<Loader>()(std::declval<decltype(*std::declval<Iterator>())>())), 4>
	{
	private:
		Filter const _filter;
		Loader const _loader;
	public:
		using base_iterator_t = Iterator;
		using value_type = decltype(*std::declval<Iterator>());
		using _Out = decltype(std::declval<Loader>()(std::declval<value_type>()));

		using handle_t = Object<Iterator, Filter, Loader, 4>;
		using base_t = Base<handle_t, base_iterator_t, _Out, 4>;
		using iterator_t = linq::iterator<handle_t, base_iterator_t, _Out, 4>;

		typedef iterator_t iterator;
		typedef iterator_t const_iterator;

	private:
		friend iterator_t;

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
			const auto new_loader = [last_loader, next_loader](value_type value) noexcept -> decltype(next_loader(std::declval<_Out>()))
			{
				return next_loader(last_loader(value));
			};
			return Object<Iterator, Filter, decltype(new_loader), 4>(
				static_cast<base_iterator_t const &>(_begin),
				static_cast<base_iterator_t const &>(_end),
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
			return Object<Iterator, decltype(new_filter), Loader, 4>(
				std::find_if(static_cast<base_iterator_t const &>(_begin), static_cast<base_iterator_t const &>(_end), new_filter),
				static_cast<base_iterator_t const &>(_end),
				new_filter,
				last_loader);
		}

	};

	template<typename Iterator, typename Loader>
	class Object<Iterator, void, Loader, 3> : public Base<Object<Iterator, void, Loader, 3>, Iterator,
		decltype(std::declval<Loader>()(std::declval<decltype(*std::declval<Iterator>())>())), 3>
	{
	private:
		Loader const _loader;
	public:
		using base_iterator_t = Iterator;
		using value_type = decltype(*std::declval<Iterator>());
		using _Out = decltype(std::declval<Loader>()(std::declval<value_type>()));

		using handle_t = Object<Iterator, void, Loader, 3>;
		using base_t = Base<handle_t, base_iterator_t, _Out, 3>;
		using iterator_t = linq::iterator<handle_t, base_iterator_t, _Out, 3>;

		typedef iterator_t iterator;
		typedef iterator_t const_iterator;
	private:
		friend iterator_t;

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
			const auto &new_loader = [last_loader, next_loader](value_type value) noexcept -> decltype(next_loader(std::declval<_Out>()))
			{
				return next_loader(last_loader(value));
			};
			return Object<Iterator, void, decltype(new_loader), 3>(
				static_cast<base_iterator_t const &>(_begin),
				static_cast<base_iterator_t const &>(_end),
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
			return Object<Iterator, decltype(new_filter), Loader, 4>(
				std::find_if(static_cast<base_iterator_t const &>(_begin), static_cast<base_iterator_t const &>(_end), new_filter),
				static_cast<base_iterator_t const &>(_end),
				new_filter,
				last_loader);
		}
	};

	template<typename Iterator, typename Filter>
	class Object<Iterator, Filter, void, 2> : public Base<Object<Iterator, Filter, void, 2>, Iterator, decltype(*std::declval<Iterator>()), 2>
	{
	private:
		Filter const _filter;
	public:
		using base_iterator_t = Iterator;
		using value_type = decltype(*std::declval<Iterator>());
		using _Out = decltype(*std::declval<Iterator>());

		using handle_t = Object<Iterator, Filter, void, 2>;
		using base_t = Base<handle_t, base_iterator_t, _Out, 2>;
		using iterator_t = linq::iterator<handle_t, base_iterator_t, _Out, 2>;

		typedef iterator_t iterator;
		typedef iterator_t const_iterator;

	private:
		friend iterator_t;

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
			return Object<Iterator, Filter, decltype(next_loader), 4>(
				static_cast<base_iterator_t const &>(_begin),
				static_cast<base_iterator_t const &>(_end),
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
			return Object<Iterator, Filter, void, 2>(
				std::find_if(static_cast<base_iterator_t const &>(_begin), static_cast<base_iterator_t const &>(_end), new_filter),
				static_cast<base_iterator_t const &>(_end),
				new_filter);
		}
	};

	template<typename Iterator>
	class Object<Iterator, void, void, 1> : public Base<Object<Iterator, void, void, 1>, Iterator, decltype(*std::declval<Iterator>()), 1>
	{
	public:
		using base_iterator_t = Iterator;
		using value_type = decltype(*std::declval<Iterator>());


		using handle_t = Object<Iterator, void, void, 1>;
		using base_t = Base<Object<Iterator, void, void, 1>, base_iterator_t, value_type, 1>;

		typedef Iterator iterator;
		typedef Iterator const_iterator;

	public:

		Object() = delete;
		~Object() = default;
		Object(Object const &) = default;
		Object(base_iterator_t const &begin, base_iterator_t const &end)
			: base_t(begin, end)
		{}

		template<typename Func>
		auto select(Func const &next_loader) const noexcept
		{
			return Object<Iterator, void, Func, 3>(
				static_cast<base_iterator_t const &>(_begin),
				static_cast<base_iterator_t const &>(_end),
				next_loader);
		}

		template<typename Func>
		auto where(Func const &next_filter) const noexcept
		{
			return Object<Iterator, Func, void, 2>(
				std::find_if(static_cast<base_iterator_t const &>(_begin), static_cast<base_iterator_t const &>(_end), next_filter),
				static_cast<base_iterator_t const &>(_end),
				next_filter);
		}
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