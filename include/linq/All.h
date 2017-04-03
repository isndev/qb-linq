#ifndef ALL_H_

namespace linq
{
	template<typename Iterator, typename Proxy>
	class All : public IState
		<
		All<Iterator, Proxy>,
		Iterator,
		decltype(*std::declval<Iterator>()),
		iterator_type::basic
		>
	{

	public:
		using proxy_t = All<Iterator, Proxy>;
		using Out = decltype(*std::declval<Iterator>());
		using base_t = IState<proxy_t, Iterator, Out, iterator_type::basic>;

		using iterator_t = linq::iterator<proxy_t, Iterator, Out, iterator_type::basic>;

		typedef iterator_t iterator;
		typedef iterator_t const_iterator;

	private:
		Proxy proxy_;
	public:

		All() = delete;
		All(All const &) = default;
		All(Iterator const &begin, Iterator const &end, Proxy proxy)
			: base_t(begin, end), proxy_(proxy)
		{}

		inline auto &operator[](int const index) const
		{
			return (*proxy_)[index];
		}

	};
}

#endif // !ALL_H_
