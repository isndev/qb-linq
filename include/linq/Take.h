#ifndef TAKE_H_
# define TAKE_H_

namespace linq
{
	template <typename Base, typename In>
	class take_it : public Base {
	public:
		typedef Base                             base;
		typedef typename Base::iterator_category iterator_category;
		typedef decltype(*std::declval<Base>())	 value_type;
		typedef typename Base::difference_type	 difference_type;
		typedef typename Base::pointer			 pointer;
		typedef value_type 	                     reference;

		take_it() = delete;
		take_it(const take_it &) = default;
		take_it(Base const &base, In const &when) noexcept(true) : Base(base), _when(when)
		{}

		constexpr auto const &operator=(take_it const &rhs) noexcept(true) {
			static_cast<Base>(*this) = static_cast<Base const &>(rhs);
			return *this;
		}
		constexpr auto const &operator++() noexcept(true) {
			static_cast<Base &>(*this).operator++();
			return (*this);
		}
		constexpr auto const operator++(int) noexcept(true) {
			auto tmp = *this;
			operator++();
			return (tmp);
		}
		constexpr auto const &operator--() noexcept(true) {
			static_cast<Base &>(*this).operator--();
			return (*this);
		}
		constexpr auto const operator--(int) noexcept(true) {
			auto tmp = *this;
			operator--();
			return (tmp);
		}
		constexpr bool operator!=(take_it const &rhs) const noexcept(true) {
			return static_cast<Base const &>(*this) != static_cast<Base const &>(rhs) 
				&& _when(*static_cast<Base const &>(*this));
		}
		constexpr bool operator==(take_it const &rhs) const noexcept(true) {
			return static_cast<Base const &>(*this) == static_cast<Base const &>(rhs)
				|| !_when(*static_cast<Base const &>(rhs));
		}
	
	private:
		In const _when;
	};

	template <typename Base>
	class take_it<Base, int> : public Base {
	public:
		typedef Base                             base;
		typedef typename Base::iterator_category iterator_category;
		typedef decltype(*std::declval<Base>())	 value_type;
		typedef typename Base::difference_type	 difference_type;
		typedef typename Base::pointer			 pointer;
		typedef value_type 	                     reference;

		take_it() = delete;
		take_it(const take_it &) = default;
		take_it(Base const &base, int const max) noexcept(true) : Base(base), max_(-max)
		{}

		constexpr auto const &operator=(take_it const &rhs) noexcept(true) {
			static_cast<Base>(*this) = static_cast<Base const &>(rhs);
			return *this;
		}
		constexpr auto const &operator++() noexcept(true) {
			static_cast<Base &>(*this).operator++();
			return (*this);
		}
		constexpr auto operator++(int) noexcept(true) {
			auto tmp = *this;
			operator++();
			return (tmp);
		}
		constexpr auto const &operator--() noexcept(true) {
			static_cast<Base &>(*this).operator--();
			return (*this);
		}
		constexpr auto operator--(int) noexcept(true) {
			auto tmp = *this;
			operator--();
			return (tmp);
		}
		constexpr bool operator!=(take_it const &rhs) const noexcept(true) {
			return  static_cast<Base const &>(*this) != static_cast<Base const &>(rhs) && max_++ < 0;
		}
		constexpr bool operator==(take_it const &rhs) const noexcept(true) {
			return  static_cast<Base const &>(*this) == static_cast<Base const &>(rhs) || max_++ >= 0;
		}
	
	private:
		mutable int max_;
	};

	template <typename Base, typename In = int>
	class Take : public TState<take_it<Base, In>> {
	public:
		typedef take_it<Base, In> iterator;
		typedef iterator const_iterator;

		using base_t = TState<iterator>;
	public:
		~Take() = default;
		Take() = delete;
		Take(Take const &rhs)
			: base_t(static_cast<base_t const &>(rhs))
		{}

		Take(Base const &begin, Base const &end, In const &in) noexcept(true)
			: base_t(iterator(begin, in), iterator(end, in))
		{}

	};
}

#endif // !TAKE_H_
