#ifndef WHERE_H_
# define WHERE_H_

namespace linq
{
	template<typename Iterator, typename Filter>
	class Where
		: public IState
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
		using base_t = IState<proxy_t, base_iterator_t, Out, iterator_type::filter>;
		using iterator_type = linq::iterator<proxy_t, base_iterator_t, Out, iterator_type::filter>;

		typedef iterator_type iterator;
		typedef iterator_type const_iterator;

	private:
		friend iterator_type;
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
			return Where<Iterator, decltype(new_filter)>(
				std::find_if(static_cast<base_iterator_t const &>(this->_begin), static_cast<base_iterator_t const &>(this->_end), new_filter),
				static_cast<base_iterator_t const &>(this->_end),
				new_filter);
		}
	};
}

#endif // !WHERE_H_
