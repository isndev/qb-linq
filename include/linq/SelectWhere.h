#ifndef SELECTWHERE_H_
# define SELECTWHERE_H_

namespace linq
{
	template<typename Iterator, typename Filter, typename Loader>
	class SelectWhere
		: public IState
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
		using base_t = IState<proxy_t, base_iterator_t, Out, iterator_type::full>;
		using iterator_t = linq::iterator<proxy_t, base_iterator_t, Out, iterator_type::full>;

		typedef iterator_t iterator;
		typedef iterator_t const_iterator;

	private:
		friend iterator_t;
		Filter const _filter;
		Loader const _loader;

		inline bool validate(base_iterator_t const &it) const noexcept
		{
			return it != static_cast<base_iterator_t const &>(this->_end)
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
}

#endif // !SELECTWHERE_H_
