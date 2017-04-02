#ifndef ISTATE_H_
# define ISTATE_H_

namespace linq
{
	template<typename Proxy, typename Base_It, typename Out, iterator_type ItType>
	class IState
	{
		using iterator_t = iterator<Proxy, Base_It, Out, ItType>;
	protected:
		iterator_t const _begin;
		iterator_t const _end;
	public:
		IState() = delete;
		~IState() = default;
		IState(IState const &rhs)
			:
			_begin(rhs._begin, static_cast<Proxy const &>(*this)),
			_end(rhs._end, static_cast<Proxy const &>(*this))
		{}
		IState(Base_It const &begin, Base_It const &end)
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
		template<typename... Funcs>
		auto selectMany(Funcs const &...loaders) const noexcept
		{
			auto const &next_loader = [loaders...] (Out val)
			{
				return std::tuple<decltype(loaders(val))...>(loaders(val)...);
			};

			return Select<Base_It, decltype(next_loader)>(
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
		template<typename... Funcs>
		auto groupBy(Funcs const &...keys) const noexcept
		{
			using group_type = group_by<Out, Funcs...>;
			using map_out = typename group_type::type;
			auto result = std::make_shared<map_out>();

			for (Out it : *this)
				group_type::emplace(*result, it, keys...);

			return GroupBy<typename map_out::iterator, decltype(result)>(result->begin(), result->end(), result);
		}
		template<typename... Funcs>
		auto orderBy(Funcs const &... keys) const noexcept
		{
			auto proxy = std::make_shared<std::vector<typename std::remove_reference<Out>::type>>();
			//std::copy(_begin, _end, std::back_inserter(*proxy));
			for (Out it : *this)
				proxy->push_back(it);
			std::sort(proxy->begin(), proxy->end(), [keys...](Out a, Out b) -> bool
			{
				return order_by(a, b, std::move(order_by_modifier_end<Out, Funcs>(keys))...);
			});

			return OrderBy<decltype(proxy->begin()), decltype(proxy)>(proxy->begin(), proxy->end(), proxy);
		}
		template<typename Func>
		auto skipWhile(Func const &func) const noexcept
		{
			auto ret = _begin;
			while (static_cast<Base_It const &>(ret) != static_cast<Base_It const &>(_end) && func(*ret))
				++ret;
			return From<iterator_t>(ret, _end);
		}
		//todo takeWhile
		auto skip(std::size_t offset) const noexcept
		{
			auto ret = _begin;
			for (std::size_t i = 0; i < offset && ret != _end; ++ret, ++i);

			return From<iterator_t>(ret, _end);
		}
		auto take(std::size_t number) const
		{
			return Take<iterator_t>(_begin, _end, number);
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
}

#endif // !ISTATE_H_
