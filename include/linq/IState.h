#ifndef ISTATE_H_
# define ISTATE_H_

namespace linq
{
	template<typename Iterator>
	class IState
	{
		using Out = typename Iterator::value_type;
	protected:
		Iterator const _begin;
		Iterator const _end;

	public:
		~IState() = default;
		IState() = delete;
		IState(IState const &rhs) = default;
		IState(Iterator const &&begin, Iterator const &&end)
			: _begin(begin), _end(end) {}

		constexpr Iterator const &begin() const noexcept(true) { return _begin; }
		constexpr Iterator const &end() const noexcept(true) { return _end; }
		constexpr Out first() const noexcept(true) { return *_begin; }
		constexpr auto firstOrDefault() const noexcept(true) {
			return any() ? first() : typename std::remove_reference<Out>::type{};
		}
		constexpr Out last() const noexcept(true) { return *std::reverse_iterator<Iterator>(_end); }
		constexpr auto lastOrDefault() const noexcept(true) {
			return any() ? last() : typename std::remove_reference<Out>::type{};
		}
		
		template<typename Func>
		constexpr auto select(Func const &next_loader) const noexcept(true) {
			return Select<Iterator, Func>(
				this->_begin,
				this->_end,
				next_loader);
		}
		template<typename... Funcs>
		constexpr auto selectMany(Funcs const &...loaders) const noexcept(true) {
			auto const &next_loader = [loaders...] (Out val)
			{
				return std::tuple<decltype(loaders(val))...>(loaders(val)...);
			};

			return Select<Iterator, decltype(next_loader)>(
				this->_begin,
				this->_end,
				next_loader);
		}
		template<typename Func>
		constexpr auto where(Func const &next_filter) const noexcept(true) {
			auto const new_begin = std::find_if(_begin, _end, next_filter);
			return Where<Iterator, Func>(new_begin,
				                         any() ? std::find_if(std::reverse_iterator<Iterator>(this->_end),
											 std::reverse_iterator<Iterator>(new_begin),
											 next_filter).base() : _end,
				                         next_filter
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
			for (Out it : *this)
				proxy->push_back(it);
			std::sort(proxy->begin(), proxy->end(), [keys...](Out a, Out b) -> bool
			{
				return order_by(a, b, std::move(order_by_modifier_end<Out, Funcs>(keys))...);
			});

			return All<decltype(proxy->begin()), decltype(proxy)>(proxy->begin(), proxy->end(), proxy);
		}

		constexpr auto skip(std::size_t const offset) const noexcept(true) {
			auto ret = _begin;
			for (std::size_t i = 0; i < offset && ret != _end; ++ret, ++i);

			return From<Iterator>(ret, _end);
		}
		template<typename Func>
		constexpr auto skipWhile(Func const &func) const noexcept(true) {
			auto ret = _begin;
			while (ret != _end && func(*ret))
				++ret;
			return From<Iterator>(ret, _end);
		}		
		constexpr auto take(int const max) const noexcept(true) {
			return Take<Iterator>(_begin, _end, max);
		}
		template<typename Func>
		constexpr auto takeWhile(Func const &func) const noexcept(true) {
			return Take<Iterator, Func>(_begin, _end, func);
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
			typename std::remove_const<typename std::remove_reference<decltype(*_begin)>::type>::type val(*_begin);
			for (Out it : *this)
				if (it > val)
					val = it;
			return val;
		}
		constexpr auto max() const noexcept(true) {
			typename std::remove_const<typename std::remove_reference<decltype(*_begin)>::type>::type val(*_begin);
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
		constexpr auto count() const noexcept(true) {
			std::size_t number{ 0 };
			for (auto it = _begin; it != _end; ++it, ++number);
			return number;
		}
		constexpr bool any() const noexcept(true) {
			return _begin != _end;
		}
	};
}

#endif // !ISTATE_H_
