#ifndef SELECT_H_
# define SELECT_H_

namespace linq
{
	template <typename Base, typename Loader>
	class select_it : public Base {
	public:
		typedef Base base;
		typedef decltype(std::declval<Loader>()(*std::declval<Base>())) out;

		select_it() = delete;
		select_it(select_it const &) = default;
		select_it(Base const &base, Loader const &loader) noexcept(true)
			: Base(base), _loader(loader) {}

		inline auto const &operator=(select_it const &rhs) noexcept(true) {
			static_cast<Base>(*this) = static_cast<Base const &>(rhs);
			return *this;
		}
		inline out operator*() const noexcept(true) {
			return _loader(*static_cast<Base const &>(*this));
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
		Select(Select const &) = default;
		Select(BaseIt const &begin, BaseIt const &end, Loader const &loader)
			: base_t(iterator(begin, loader), iterator(end, loader)) {}
	};
}

#endif // !SELECT_H_
