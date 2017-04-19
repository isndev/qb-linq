#ifndef ORDERBY_H_
# define ORDERBY_H_

namespace linq
{
	template<typename Iterator, typename Proxy>
	class OrderBy : public IState
		<
		OrderBy<Iterator, Proxy>,
		Iterator,
		decltype(*std::declval<Iterator>()),
		iterator_type::basic
		>
	{

	public:
		using proxy_t = OrderBy<Iterator, Proxy>;
		using Out = decltype(*std::declval<Iterator>());
		using base_t = IState<proxy_t, Iterator, Out, iterator_type::basic>;

		using iterator_type = linq::iterator<proxy_t, Iterator, Out, iterator_type::basic>;

		typedef iterator_type iterator;
		typedef iterator_type const_iterator;

	private:
		Proxy proxy_;
	public:

		OrderBy() = delete;
		OrderBy(OrderBy const &) = default;
		OrderBy(Iterator const &begin, Iterator const &end, Proxy proxy)
			: base_t(begin, end), proxy_(proxy)
		{}

		inline auto asc() const noexcept { return *this; }
		inline auto desc() const noexcept { return OrderBy<decltype(proxy_->rbegin()), Proxy>(proxy_->rbegin(), proxy_->rend(), proxy_); }

	};
}

#endif // !ORDERBY_H_
