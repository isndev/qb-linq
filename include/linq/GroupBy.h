#ifndef GROUPBY_H_
# define GROUPBY_H_

namespace linq
{
	template<typename Iterator, typename Proxy>
	class GroupBy : public IState
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
		using base_t = IState<proxy_t, Iterator, Out, iterator_type::basic>;

		using iterator_type = linq::iterator<proxy_t, Iterator, Out, iterator_type::basic>;

		typedef iterator_type iterator;
		typedef iterator_type const_iterator;

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

		inline auto &operator[](last_key_t const &key) const
		{
			return (*proxy_)[key];
		}

	};
}

#endif // !GROUPBY_H_
