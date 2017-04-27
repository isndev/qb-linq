#ifndef IENUMERABLE_H_
# define IENUMERABLE_H_

namespace linq
{
	template <typename Handle>
	class IEnumerable : public Handle
	{
		typedef typename Handle::iterator iterator_type;
		typedef decltype(*std::declval<iterator_type>()) out_t;
	
	public:
		IEnumerable() = delete;
		~IEnumerable() = default;
		IEnumerable(IEnumerable const &rhs)
			: Handle(static_cast<Handle const &>(rhs))
		{}
		IEnumerable(Handle const &rhs)
			: Handle(rhs)
		{}

		template<typename Func>
		constexpr auto Select(Func const &next_loader) const noexcept(true) {
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).select(next_loader))>;
			return ret_t(static_cast<Handle const &>(*this).select(next_loader));
		}
		template<typename... Funcs>
		constexpr auto SelectMany(Funcs const &...keys) const noexcept(true) {
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).selectMany(keys...))>;
			return ret_t(static_cast<Handle const &>(*this).selectMany(keys...));
		}
		template<typename Func>
		constexpr auto Where(Func const &next_filter) const noexcept(true) {
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).where(next_filter))>;
			return ret_t(static_cast<Handle const &>(*this).where(next_filter));
		}
		template<typename... Funcs>
		constexpr auto GroupBy(Funcs const &...keys) const noexcept(true) {
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).groupBy(keys...))>;
			return ret_t(static_cast<Handle const &>(*this).groupBy(keys...));
		}
		template<typename... Funcs>
		constexpr auto OrderBy(Funcs const &...keys) const noexcept(true) {
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).orderBy(keys...))>;
			return ret_t(static_cast<Handle const &>(*this).orderBy(keys...));
		}
		template<typename Func>
		constexpr auto SkipWhile(Func const &func) const noexcept(true) {
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).skipWhile(func))>;
			return ret_t(static_cast<Handle const &>(*this).skipWhile(func));
		}
		template<typename Func>
		constexpr auto TakeWhile(Func const &func) const noexcept(true) {
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).takeWhile(func))>;
			return ret_t(static_cast<Handle const &>(*this).takeWhile(func));
		}

		constexpr auto Asc() const noexcept(true) {
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).asc())>;
			return ret_t(static_cast<Handle const &>(*this).asc());
		}
		constexpr auto Desc() const noexcept(true) {
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).desc())>;
			return ret_t(static_cast<Handle const &>(*this).desc());
		}
		constexpr auto Skip(std::size_t const offset) const noexcept(true) {
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).skip(offset))>;
			return ret_t(static_cast<Handle const &>(*this).skip(offset));
		}
		constexpr auto Take(int const limit) const noexcept(true) {
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).take(limit))>;
			return ret_t(static_cast<Handle const &>(*this).take(limit));
		}
		constexpr auto All() const noexcept(true) {
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).all())>;
			return ret_t(static_cast<Handle const &>(*this).all());
		}
		constexpr auto Min() const noexcept(true) {
			return static_cast<Handle const &>(*this).min();
		}
		constexpr auto Max() const noexcept(true) {
			return static_cast<Handle const &>(*this).max();
		}
		constexpr auto Sum() const noexcept(true) {
			return static_cast<Handle const &>(*this).sum();
		}
		constexpr auto Count() const noexcept(true) {
			return static_cast<Handle const &>(*this).count();
		}
		constexpr bool Any() const noexcept(true) {
			return static_cast<Handle const &>(*this).any();
		}
		constexpr const iterator_type &begin() const noexcept(true) {
			return static_cast<Handle const &>(*this).begin();
		}
		constexpr const iterator_type &end() const noexcept(true) {
			return static_cast<Handle const &>(*this).end();
		}
		constexpr const out_t First() const noexcept(true) {
			return static_cast<Handle const &>(*this).first();
		}
		constexpr const auto FirstOrDefault() const noexcept(true) {
			return static_cast<Handle const &>(*this).firstOrDefault();
		}
		constexpr const out_t Last() const noexcept(true) {
			return static_cast<Handle const &>(*this).last();
		}
		constexpr const auto LastOrDefault() const noexcept(true) {
			return static_cast<Handle const &>(*this).lastOrDefault();
		}

		template<typename Key>
		constexpr auto &operator[](Key const &key) const {
			return static_cast<Handle const &>(*this).operator[](key);
		}
	};
}

#endif // !IENUMERABLE_H_
