#ifndef WHERE_H_
# define WHERE_H_

namespace linq
{
	template <typename Base, typename Filter>
	class where_it : public Base {
	public:
		typedef Base base;
		typedef decltype(*std::declval<Base>()) out;

		where_it() = delete;
		where_it(where_it const &) = default;
		where_it(Base const &base, Base const &end, Filter const &filter) noexcept(true)
			: Base(base), _end(end), _filter(filter) {}
		
		inline auto const &operator=(where_it const &rhs) noexcept(true) {
			static_cast<Base>(*this) = static_cast<Base const &>(rhs);
			return *this;
		}
		inline auto const &operator++() noexcept(true) {
			do
			{
				static_cast<Base &>(*this).operator++();
			} while (static_cast<Base const &>(*this) != _end && !_filter(*static_cast<Base const &>(*this)));
			return *this;
		}

	private:
		Base const _end;
		Filter const _filter;
	};

	template<typename BaseIt, typename Filter>
	class Where : public IState<where_it<BaseIt, Filter>> {
	public:
		typedef where_it<BaseIt, Filter> iterator;
		typedef iterator const_iterator;

		using base_t = IState<iterator>;
	public:
		~Where() = default;
		Where() = delete;
		Where(Where const &) = default;
		Where(BaseIt const &begin, BaseIt const &end, Filter const &filter) noexcept(true)
			: base_t(iterator(begin, end, filter), iterator(end, end, filter)) {}
	};
}

#endif // !WHERE_H_
