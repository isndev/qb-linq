#ifndef TAKE_H_
# define TAKE_H_

namespace linq
{
	template <typename Iterator>
	class Take : public IState
		<
		Take<Iterator>,
		Iterator,
		decltype(*std::declval<Iterator>()),
		iterator_type::reach
		>
	{
	public:
		using Out = decltype(*std::declval<Iterator>());

		using proxy_t = Take<Iterator>;
		using base_t = IState
			<
			Take<Iterator>,
			Iterator,
			Out,
			iterator_type::reach
			>;
		using iterator_t = linq::iterator<proxy_t, Iterator, Out, iterator_type::reach>;

		typedef iterator_t iterator;
		typedef iterator_t const_iterator;
	private:
		friend iterator_t;

		mutable std::size_t number_;
		inline bool reach(Iterator const &) const noexcept { return number_ > 0; }
		inline void incr(Iterator const &) const noexcept { --number_; }

	public:
		Take() = delete;
		~Take() = default;
		Take(Take const &) = default;

		Take(Iterator const &begin, Iterator const &end, std::size_t number)
			: base_t(begin, end), number_(number)
		{}

		auto take(std::size_t number) const
		{
			return Take<Iterator>(this->_begin, this->_end, number);
		}
	};
}

#endif // !TAKE_H_
