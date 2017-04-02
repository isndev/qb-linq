#pragma once

#include <iostream>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>
#include <algorithm>

//version withouth
//version only filter

namespace linq
{

	template<typename _Handle, typename _Base, int _Level = 1>
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

		inline auto operator*() const noexcept { return std::move(_handle._converter(*static_cast<_Base const &>(*this))); }

		iterator &operator++() noexcept {
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
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(_Base const &base, _Handle const &handle)
			: _Base(base), _handle(handle)
		{}

		inline auto operator*() const noexcept { return std::move(_handle._converter(*static_cast<_Base const &>(*this))); }

		inline iterator &operator++() noexcept {
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
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(_Base const &base, _Handle const &handle)
			: _Base(base), _handle(handle)
		{}

		inline auto operator*() const noexcept { return *static_cast<_Base const &>(*this); }
		//inline auto &operator*() { return *static_cast<_Base &>(*this); }

		iterator &operator++() noexcept {
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
		iterator(_Base const &base, _Handle const &)
			: _Base(base)
		{}
	};

	template<typename _Handle, typename _Base_It, int _Level>
	class Base
	{
		using iterator_t = iterator<_Handle, _Base_It, _Level>;
	protected:
		iterator_t const _begin;
		iterator_t const _end;
	public:
		Base() = delete;
		~Base() = default;
		Base(Base const &) = default;

		template<int Level = _Level>
		Base(_Base_It const &begin, _Base_It const &end)
			:
			_begin(begin, static_cast<_Handle const &>(*this)),
			_end(end, static_cast<_Handle const &>(*this))
		{}

		inline iterator_t const begin() const noexcept { return _begin; }
		inline iterator_t const end() const noexcept { return _end; }


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

		auto sum()
		{
			decltype(*_begin) result{};
			for (auto it : *this)
				result += it;
			return result;
		}

	};


	/*Linq Object with Filter and Converter*/
	template<typename _Container, typename _Out = void, int _Level = 1>
	class Object : public Base<Object<_Container, _Out, 4>, typename _Container::iterator, 4>
	{
	public:
		using base_iterator_t = typename _Container::iterator;
		using value_type = typename _Container::value_type;

		using handle_t = Object<_Container, _Out, _Level>;
		using base_t = Base<Object<_Container, _Out, 4>, base_iterator_t, 4>;
		using iterator_t = iterator<handle_t, base_iterator_t, _Level>;
		using base_filter = std::function<bool(value_type &)>;
		using base_converter = std::function<_Out(value_type &)>;

	protected:
		base_filter const _filter;
		base_converter const _converter;

	private:
		friend iterator_t;

	public:
		Object() = delete;
		~Object() = default;
		Object(Object const &) = default;
		Object(base_iterator_t const &begin, base_iterator_t const &end,
			std::function<bool(value_type &)> const &filter,
			std::function<_Out(value_type &)> const &converter)
			: base_t(begin, end), _filter(filter), _converter(converter)
		{}

		template<typename New_Out_t>
		auto select(std::function<New_Out_t(_Out &)> const &next_converter) const noexcept
		{
			const auto &last_converter = _converter;
			return Object<_Container, New_Out_t, 4>(
				static_cast<base_iterator_t const &>(_begin),
				static_cast<base_iterator_t const &>(_end),
				_filter,
				[last_converter, next_converter](value_type &value) noexcept -> New_Out_t
			    {
				    auto tmp = last_converter(value);
				    return next_converter(tmp);
			    });
		}

		auto where(std::function<bool(_Out &)> const &next_filter) const noexcept
		{
			const auto &last_converter = _converter;
			const auto &last_filter = _filter;
			const auto &new_filter = [last_converter, last_filter, next_filter](value_type &value) noexcept -> bool
			{
				auto tmp = last_converter(value);
				return last_filter(value) && next_filter(tmp);
			};
			return Object<_Container, _Out, 4>(
				std::find_if(static_cast<base_iterator_t const &>(_begin), static_cast<base_iterator_t const &>(_end), new_filter),
				static_cast<base_iterator_t const &>(_end),
				new_filter,
				last_converter);
		}

	};

	template<typename _Container, typename _Out>
	class Object<_Container, _Out, 3> : public Base<Object<_Container, _Out, 3>, typename _Container::iterator, 3>
	{
	public:
		using base_iterator_t = typename _Container::iterator;
		using value_type = typename _Container::value_type;

		using handle_t = Object<_Container, _Out, 3>;
		using base_t = Base<Object<_Container, _Out, 3>, base_iterator_t, 3>;
		using iterator_t = iterator<handle_t, base_iterator_t, 3>;
		using base_converter = std::function<_Out(value_type &)>;

	protected:
		base_converter const _converter;

	private:
		friend iterator_t;

	public:
		Object() = delete;
		~Object() = default;
		Object(Object const &) = default;
		Object(base_iterator_t const &begin, base_iterator_t const &end,
			std::function<_Out(value_type &)> const &converter)
			: base_t(begin, end), _converter(converter)
		{}

		template<typename New_Out_t>
		auto select(std::function<New_Out_t(_Out &)> const &next_converter) const noexcept
		{
			const auto &last_converter = _converter;
			return Object<_Container, New_Out_t, 3>(
				static_cast<base_iterator_t const &>(_begin),
				static_cast<base_iterator_t const &>(_end),
				[last_converter, next_converter](value_type &value) noexcept -> New_Out_t
			{
				auto tmp = last_converter(value);
				return next_converter(tmp);
			});
		}

		auto where(std::function<bool (_Out &)> const &next_filter) const noexcept
		{
			const auto &last_converter = _converter;
			const auto &new_filter = [last_converter, next_filter](value_type &value) noexcept -> bool
			{
				auto tmp = last_converter(value);
				return next_filter(tmp);
			};
			return Object<_Container, _Out, 4>(
				std::find_if(static_cast<base_iterator_t const &>(_begin), static_cast<base_iterator_t const &>(_end), new_filter),
				static_cast<base_iterator_t const &>(_end),
				new_filter,
				last_converter);
		}
	};

	template<typename _Container>
	class Object<_Container, void, 2> : public Base<Object<_Container, void, 2>, typename _Container::iterator, 2>
	{
	public:
		using base_iterator_t = typename _Container::iterator;
		using value_type = typename _Container::value_type;

		using handle_t = Object<_Container, void, 2>;
		using base_t = Base<Object<_Container, void, 2>, base_iterator_t, 2>;
		using iterator_t = iterator<handle_t, base_iterator_t, 2>;
		using base_filter = std::function<bool(value_type &)>;

	protected:
		base_filter const _filter;

	private:
		friend iterator_t;

	public:
		Object() = delete;
		~Object() = default;
		Object(Object const &) = default;
		Object(base_iterator_t const &begin, base_iterator_t const &end,
			std::function<bool(value_type &)> const &filter)
			: base_t(begin, end), _filter(filter)
		{}

		template<typename New_Out_t>
		auto select(std::function<New_Out_t(value_type &)> const &next_converter) const noexcept
		{
			return Object<_Container, New_Out_t, 4>(
				static_cast<base_iterator_t const &>(_begin),
				static_cast<base_iterator_t const &>(_end),
				_filter,
				next_converter);
		}

		auto where(std::function<bool(value_type &)> const &next_filter) const noexcept
		{
			const auto &last_filter = _filter;
			const auto &new_filter = [last_filter, next_filter](value_type &value) noexcept -> bool
			{
				return last_filter(value) && next_filter(value);
			};
			return Object<_Container, void, 2>(
				std::find_if(static_cast<base_iterator_t const &>(_begin), static_cast<base_iterator_t const &>(_end), new_filter),
				static_cast<base_iterator_t const &>(_end),
				new_filter);
		}
	};

	template<typename _Container>
	class Object<_Container, void, 1> : public Base<Object<_Container, void, 1>, typename _Container::iterator, 1>
	{
	public:
		typedef typename _Container::iterator base_iterator_t;
		typedef typename _Container::value_type value_type;

		using handle_t = Object<_Container, void, 1>;
		using base_t = Base<Object<_Container, void, 1>, base_iterator_t, 1>;

	public:

		Object() = delete;
		~Object() = default;
		Object(Object const &) = default;
		Object(base_iterator_t const &begin, base_iterator_t const &end)
			: base_t(begin, end)
		{}

		template<typename New_Out_t>
		auto select(std::function<New_Out_t(value_type &)> const &next_converter) const noexcept
		{
			return Object<_Container, New_Out_t, 3>(
				static_cast<base_iterator_t const &>(_begin),
				static_cast<base_iterator_t const &>(_end),
				next_converter);
		}

		auto where(std::function<bool(value_type &)> const &next_filter) const noexcept
		{
			return Object<_Container, void, 2>(
				std::find_if(static_cast<base_iterator_t const &>(_begin), static_cast<base_iterator_t const &>(_end), next_filter),
				static_cast<base_iterator_t const &>(_end),
				next_filter);
		}
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