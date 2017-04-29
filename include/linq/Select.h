#ifndef SELECT_H_
# define SELECT_H_

namespace linq
{
	template <typename Base, typename Loader>
	class select_it : public Base {
	public:
		typedef Base base;
		typedef typename Base::iterator_category							iterator_category;
		typedef decltype(std::declval<Loader>()(*std::declval<Base>()))		value_type;
		typedef typename Base::difference_type								difference_type;
		typedef typename Base::pointer										pointer;
		typedef value_type 													reference;

		select_it() = delete;
		select_it(select_it const &rhs) = default;
		select_it(Base const &base, Loader const &loader) noexcept(true)
			: Base(base), _loader(loader)
		{}

		constexpr auto const &operator=(select_it const &rhs) noexcept(true) {
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
		constexpr value_type operator*() const noexcept(true) {
			return _loader(*static_cast<Base const &>(*this));
		}
		constexpr value_type operator->() const noexcept(true) {
			return *(*this);
		}
	
	private:
		Loader const _loader;
	};

	template<typename BaseIt, typename Loader>
	class Select
		: public IState<select_it<BaseIt, Loader>>
	{
	public:
		typedef select_it<BaseIt, Loader> iterator;
		typedef iterator const_iterator;

		using base_t = IState<iterator>;
	public:
		~Select() = default;
		Select() = delete;
		Select(Select const &rhs)
			: base_t(static_cast<base_t const &>(rhs))
		{}

		Select(BaseIt const &begin, BaseIt const &end, Loader const &loader)
			: base_t(iterator(begin, loader), iterator(end, loader))
		{}
	};
}

#endif // !SELECT_H_
