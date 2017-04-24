#ifndef TAKE_H_
# define TAKE_H_

namespace linq
{
	template <typename Base, typename In>
	class take_it : public Base {
	public:
		typedef Base base;
		typedef decltype(*std::declval<Base>()) out;

		take_it() = delete;
		take_it(const take_it &) = default;
		take_it(Base const &base, In const &when) noexcept(true) : Base(base), _when(when) {}

		inline auto const &operator=(Base const &rhs) noexcept(true) {
			static_cast<Base>(*this) = static_cast<Base const &>(rhs);
			return *this;
		}
		inline bool operator!=(take_it const &rhs) const noexcept(true) {
			return static_cast<Base const &>(*this) != static_cast<Base const &>(rhs) 
				&& _when(*static_cast<Base const &>(rhs));
		}

	private:
		In const _when;
	};

	template <typename Base>
	class take_it<Base, int> : public Base {
	public:
		typedef Base base;
		typedef decltype(*std::declval<Base>()) out;

		take_it() = delete;
		take_it(const take_it &) = default;
		take_it(Base const &base, int const max) noexcept(true) : Base(base), _max(-max) {}

		inline auto const &operator=(take_it const &rhs) noexcept(true) {
			static_cast<Base>(*this) = static_cast<Base const &>(rhs);
			return *this;
		}
		inline bool operator!=(take_it const &rhs) const noexcept(true) {
			return  static_cast<Base const &>(*this) != static_cast<Base const &>(rhs) && _max++ < 0;
		}
	
	private:
		mutable int _max;
	};

	template <typename BaseIt, typename In = int>
	class Take : public IState<take_it<BaseIt, In>> {
	public:
		typedef take_it<BaseIt, In> iterator;
		typedef iterator const_iterator;

		using base_t = IState<iterator>;
	public:
		~Take() = default;
		Take() = delete;
		Take(Take const &) = default;
		Take(BaseIt const &begin, BaseIt const &end, In const &in) noexcept(true)
			: base_t(iterator(begin, in), iterator(end, in)) {}
	};
}

#endif // !TAKE_H_
