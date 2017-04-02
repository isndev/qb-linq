#pragma once

#include <iostream>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>
#include <algorithm>

//version withouth
//version only filter
//verison only converter
//version full

template <int level = 0>
class Iterator
{};

template <>
class Iterator<0>
{};

template <>
class Iterator<1>
{};

template <>
class Iterator<2>
{};

// filter and converter
template <>
class Iterator<3>
{};

namespace linq
{

	template<typename _Handle, typename _Base, int _Level = 1>
	class iterator : public _Base
	{
		_Handle const &_handle;
	public:
		iterator() = delete;
		iterator(_Base const &base, _Handle const &handle)
			: _Base(base), _handle(handle)
		{}

		inline auto operator*() const { return std::move(_handle._converter(*static_cast<_Base const &>(*this))); }

		iterator &operator++() {
			do
			{
				static_cast<_Base &>(*this).operator++();
			} while (static_cast<_Base const &>(*this) != static_cast<_Base const &>(_handle._end) && !_handle._filter(*static_cast<_Base const &>(*this)));
			return (*this);
		}
	};

	template<typename _Handle, typename _Base>
	class iterator<_Handle, _Base, 3> : public _Base
	{
		_Handle const &_handle;
	public:
		iterator() = delete;
		iterator(_Base const &base, _Handle const &handle)
			: _Base(base), _handle(handle)
		{}

		inline auto operator*() const { return std::move(_handle._converter(*static_cast<_Base const &>(*this))); }

		inline iterator &operator++() {
			static_cast<_Base &>(*this).operator++();
			return *this;
		}
	};

	template<typename _Handle, typename _Base>
	class iterator<_Handle, _Base, 2> : public _Base
	{
		_Handle const &_handle;
	public:
		iterator() = delete;
		iterator(_Base const &base, _Handle const &handle)
			: _Base(base), _handle(handle)
		{}

		inline auto operator*() const { return *static_cast<_Base const &>(*this); }

		iterator &operator++() {
			do
			{
				static_cast<_Base &>(*this).operator++();
			} while (static_cast<_Base const &>(*this) != static_cast<_Base const &>(_handle._end) && !_handle._filter(*static_cast<_Base const &>(*this)));
			return (*this);
		}
	};

	template<typename _Handle, typename _Base>
	class iterator<_Handle, _Base, 1> : public _Base
	{
	public:
		iterator(_Base const &base)
			: _Base(base)
		{}
	};

	/*Linq Object with Filter and Converter*/
	template<typename _Container, typename _Out = void, int _Level = 1>
	class Object
	{
	public:
		using type = Object<_Container, _Out, _Level>;
		using base_iterator_t = typename _Container::iterator;
		using iterator_t = iterator<type, base_iterator_t, _Level>;
		using value_type = typename _Container::value_type;


		typedef typename std::function<bool(value_type &)> base_filter;
		typedef typename std::function<_Out(value_type &)> base_converter;

	protected:
		base_filter const _filter;
		base_converter const _converter;
		iterator_t const _begin;
		iterator_t const _end;
	private:
		friend iterator_t;
	public:

		Object() = delete;
		Object(base_iterator_t const &begin, base_iterator_t const &end, std::function<bool(value_type &)> const &filter, std::function<_Out(value_type &)> const &converter)
			: _filter(filter), _converter(converter),
			_begin(begin, *this), _end(end, *this)
		{}

		template<typename New_Out_t>
		auto select(std::function<New_Out_t(_Out &)> const &next_converter) const
		{
			const auto &cur_converter = _converter;
			const auto &new_converter = [cur_converter, next_converter](value_type &value) -> New_Out_t
			{
				auto tmp = cur_converter(value);
				return next_converter(tmp);
			};
			return Object<_Container, New_Out_t, 4>(static_cast<base_iterator_t>(_begin), static_cast<base_iterator_t>(_end), _filter, new_converter);
		}

		auto where(std::function<bool(_Out &)> const &next_filter) const
		{
			const auto &cur_converter = _converter;
			const auto &cur_filter = _filter;
			const auto &new_filter = [cur_converter, cur_filter, next_filter](value_type &value) -> bool
			{
				auto tmp = cur_converter(value);
				return cur_filter(value) && next_filter(tmp);
			};
			return Object<_Container, _Out, 4>(std::find_if(static_cast<base_iterator_t>(_begin), static_cast<base_iterator_t>(_end), new_filter), static_cast<base_iterator_t>(_end),
				new_filter, cur_converter);
		}

		inline iterator_t const begin() const { return _begin; }
		inline iterator_t const end() const { return _end; }

		/* export */
		template<typename _OutContainer>
		_OutContainer to()
		{
			_OutContainer out;
			std::copy(_begin, _end, std::back_inserter(out));
			return std::move(out);
		}

		template<typename _OutContainer>
		inline void to(_OutContainer &out)
		{
			std::copy(_begin, _end, std::back_inserter(out));
		}
	};

	template<typename _Container, typename _Out>
	class Object<_Container, _Out, 3>
	{
	public:
		using type = Object<_Container, _Out, 3>;
		using base_iterator_t = typename _Container::iterator;
		using iterator_t = iterator<type, base_iterator_t, 3>;
		using value_type = typename _Container::value_type;

		typedef typename std::function<_Out(value_type &)> base_converter;

	protected:
		base_converter const _converter;
		iterator_t const _begin;
		iterator_t const _end;
	private:
		friend iterator_t;
	public:

		Object() = delete;
		Object(base_iterator_t const &begin, base_iterator_t const &end, std::function<_Out(value_type &)> const &converter)
			: _converter(converter),
			_begin(begin, *this), _end(end, *this)
		{}

		template<typename New_Out_t>
		auto select(std::function<New_Out_t(_Out &)> const &next_converter) const
		{
			const auto &cur_converter = _converter;
			const auto &new_converter = [cur_converter, next_converter](value_type &value) -> New_Out_t
			{
				auto tmp = cur_converter(value);
				return next_converter(tmp);
			};
			return Object<_Container, New_Out_t, 3>(static_cast<base_iterator_t>(_begin), static_cast<base_iterator_t>(_end), new_converter);
		}

		//template<typename _Pred>
		auto where(std::function<bool (_Out &)> const &next_filter) const
		{
			const auto &cur_converter = _converter;
			const auto &new_filter = [cur_converter, next_filter](value_type &value) -> bool
			{
				auto tmp = cur_converter(value);
				return next_filter(tmp);
			};
			return Object<_Container, _Out, 4>(std::find_if(static_cast<base_iterator_t>(_begin), static_cast<base_iterator_t>(_end), new_filter), static_cast<base_iterator_t>(_end),
				new_filter, cur_converter);
		}

		inline iterator_t const begin() const { return _begin; }
		inline iterator_t const end() const { return _end; }

		/* export */
		template<typename _OutContainer>
		_OutContainer to() const
		{
			_OutContainer out;
			std::copy(_begin, _end, std::back_inserter(out));
			return std::move(out);
		}

		template<typename _OutContainer>
		inline void to(_OutContainer &out) const
		{
			std::copy(_begin, _end, std::back_inserter(out));
		}
	};

	template<typename _Container>
	class Object<_Container, void, 2>
	{
	public:
		using type = Object<_Container, void, 2>;
		using base_iterator_t = typename _Container::iterator;
		using iterator_t = iterator<type, base_iterator_t, 2>;
		using value_type = typename _Container::value_type;


		typedef typename std::function<bool(value_type &)> base_filter;

	protected:
		base_filter const _filter;
		iterator_t const _begin;
		iterator_t const _end;
	private:
		friend iterator_t;
	public:

		Object() = delete;
		Object(base_iterator_t const &begin, base_iterator_t const &end, std::function<bool(value_type &)> const &filter)
			: _filter(filter),
			_begin(begin, *this), _end(end, *this)
		{}

		template<typename New_Out_t>
		auto select(std::function<New_Out_t(value_type &)> const &next_converter) const
		{
			return Object<_Container, New_Out_t, 4>(static_cast<base_iterator_t>(_begin), static_cast<base_iterator_t>(_end), _filter, next_converter);
		}

		auto where(std::function<bool(value_type &)> const &next_filter) const
		{
			const auto &cur_filter = _filter;
			const auto &new_filter = [cur_filter, next_filter](value_type &value) -> bool
			{
				return cur_filter(value) && next_filter(value);
			};
			return Object<_Container, void, 2>(std::find_if(static_cast<base_iterator_t const &>(_begin), static_cast<base_iterator_t const &>(_end), new_filter), static_cast<base_iterator_t const &>(_end),
				new_filter);
		}

		inline iterator_t const begin() const { return _begin; }
		inline iterator_t const end() const { return _end; }

		/* export */
		template<typename _OutContainer>
		_OutContainer to() const
		{
			_OutContainer out;
			std::copy(_begin, _end, std::back_inserter(out));
			return std::move(out);
		}

		template<typename _OutContainer>
		inline void to(_OutContainer &out) const
		{
			std::copy(_begin, _end, std::back_inserter(out));
		}
	};

	template<typename _Container>
	class Object<_Container, void, 1>
	{
	public:
		using type = Object<_Container, void, 1>;
		typedef typename _Container::iterator base_iterator_t;
		using iterator_t = iterator<type, base_iterator_t, 1>;
		typedef typename _Container::value_type value_type;

	protected:
		iterator_t const _begin;
		iterator_t const _end;
	public:

		Object() = delete;
		Object(base_iterator_t const &begin, base_iterator_t const &end)
			: _begin(begin), _end(end)
		{}

		template<typename New_Out_t>
		auto select(std::function<New_Out_t(value_type &)> const &next_converter) const
		{
			return Object<_Container, New_Out_t, 3>(
				static_cast<base_iterator_t const &>(_begin),
				static_cast<base_iterator_t const &>(_end),
				next_converter);
		}

		auto where(std::function<bool(value_type &)> const &next_filter) const
		{
			return Object<_Container, void, 2>(
				std::find_if(static_cast<base_iterator_t const &>(_begin), static_cast<base_iterator_t const &>(_end), next_filter),
				static_cast<base_iterator_t const &>(_end),
				next_filter);
		}

		inline iterator_t const begin() const { return _begin; }
		inline iterator_t const end() const { return _end; }
	};

	template<typename T>
	auto from(T &container)
	{
		return std::move(linq::Object<T>(container.begin(), container.end()));
	}

	template<typename _Out, typename _Container, typename _Pred>
	auto select(_Container &cont, _Pred const &converter)
	{
		return std::move(linq::Object<_Container, _Out, 3>(
			cont.begin(),
			cont.end(),
			converter));
	}

}