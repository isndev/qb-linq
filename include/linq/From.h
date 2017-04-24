#ifndef FROM_H_
# define FROM_H_

namespace linq
{
	template<typename Base>
	class basic_it : public Base {
	public:
		typedef Base base;
		typedef decltype(*std::declval<Base>()) out;

		basic_it() = delete;
		basic_it(basic_it const &) = default;
		basic_it(Base const &base) noexcept(true)
			: Base(base) {}
		inline auto const &operator=(basic_it const &rhs) noexcept(true) {
			static_cast<Base>(*this) = static_cast<Base const &>(rhs);
			return *this;
		}
	};

	template<typename BaseIt>
	class From
		: public IState<basic_it<BaseIt>>
	{
	public:
		typedef basic_it<BaseIt> iterator;
		typedef iterator const_iterator;

		using base_t = IState<iterator>;
	public:
		~From() = default;
		From() = delete;
		From(From const &) = default;
		From(BaseIt const &begin, BaseIt const &end) noexcept(true)
			: base_t(begin, end) {}
	};
}

#endif // !FROM_H_
