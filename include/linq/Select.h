#ifndef SELECT_H_
# define SELECT_H_

namespace linq
{
	template<typename Iterator, typename Loader>
	class Select
		: public IState
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
		using base_t = IState<proxy_t, base_iterator_t, Out, iterator_type::load>;
		using iterator_type = linq::iterator<proxy_t, base_iterator_t, Out, iterator_type::load>;

		typedef iterator_type iterator;
		typedef iterator_type const_iterator;
	private:
		friend iterator_type;
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
}

#endif // !SELECT_H_
