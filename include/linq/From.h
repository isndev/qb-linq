#ifndef FROM_H_
# define FROM_H_

namespace linq
{
	template<typename Iterator>
	class From
		: public IState
		<
		From<Iterator>,
		Iterator,
		decltype(*std::declval<Iterator>()),
		iterator_type::basic
		>
	{
	public:
		using base_iterator_t = Iterator;
		using value_type = decltype(*std::declval<Iterator>());


		using proxy_t = From<Iterator>;
		using base_t = IState<proxy_t, base_iterator_t, value_type, iterator_type::basic>;

		typedef Iterator iterator;
		typedef Iterator const_iterator;

	public:

		From() = delete;
		~From() = default;
		From(From const &) = default;
		From(base_iterator_t const &begin, base_iterator_t const &end)
			: base_t(begin, end)
		{}

	};
}

#endif // !FROM_H_
