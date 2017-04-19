#ifndef FROM_H_
# define FROM_H_

namespace linq
{
	struct from_trait {};
	template<typename Iterator>
	class From
		: public IState
		<
		Iterator,
		from_trait,
		decltype(*std::declval<Iterator>()),
		eIteratorType::basic
		>
	{
	public:

		using base_iterator_t = Iterator;
		using value_type = decltype(*std::declval<Iterator>());

		using proxy_t = From<Iterator>;
		using base_t = IState<base_iterator_t, from_trait, value_type, eIteratorType::basic>;

		typedef Iterator iterator;
		typedef Iterator const_iterator;
		typedef from_trait trait;
	public:
		From() = delete;
		~From() = default;
		From(From const &) = default;
		From(base_iterator_t const &begin, base_iterator_t const &end)
			: base_t(begin, end, from_trait())
		{}

	};
}

#endif // !FROM_H_
