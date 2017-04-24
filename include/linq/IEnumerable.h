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
		IEnumerable(IEnumerable const &) = default;
		IEnumerable(Handle const &rhs)
			: Handle(rhs) {}

		template<typename Func>
		inline auto Select(Func const &next_loader) const noexcept(true)
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).select(next_loader))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).select(next_loader)));
		}
		template<typename... Funcs>
		inline auto SelectMany(Funcs const &...keys) const noexcept(true)
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).selectMany(keys...))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).selectMany(keys...)));
		}
		template<typename Func>
		inline auto Where(Func const &next_filter) const noexcept(true)
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).where(next_filter))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).where(next_filter)));
		}
		template<typename... Funcs>
		inline auto GroupBy(Funcs const &...keys) const noexcept(true)
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).groupBy(keys...))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).groupBy(keys...)));
		}
		template<typename... Funcs>
		inline auto OrderBy(Funcs const &...keys) const noexcept(true)
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).orderBy(keys...))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).orderBy(keys...)));
		}
		template<typename Func>
		inline auto SkipWhile(Func const &func) const noexcept(true)
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).skipWhile(func))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).skipWhile(func)));
		}
		template<typename Func>
		inline auto TakeWhile(Func const &func) const noexcept(true)
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).takeWhile(func))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).takeWhile(func)));
		}

		inline auto Asc() const noexcept(true)
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).asc())>;
			return ret_t(std::move(static_cast<Handle const &>(*this).asc()));
		}
		inline auto Desc() const noexcept(true)
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).desc())>;
			return ret_t(std::move(static_cast<Handle const &>(*this).desc()));
		}

		inline auto Skip(std::size_t const offset) const noexcept(true)
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).skip(offset))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).skip(offset)));
		}
		inline auto Take(int const limit) const noexcept(true)
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).take(limit))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).take(limit)));
		}
		inline auto All() const noexcept(true)
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).all())>;
			return ret_t(std::move(static_cast<Handle const &>(*this).all()));
		}
		inline auto Min() const noexcept(true)
		{
			return std::move(static_cast<Handle const &>(*this).min());
		}
		inline auto Max() const noexcept(true)
		{
			return std::move(static_cast<Handle const &>(*this).max());
		}
		inline auto Sum() const noexcept(true)
		{
			return std::move(static_cast<Handle const &>(*this).sum());
		}
		inline auto Count() const noexcept(true)
		{
			return std::move(static_cast<Handle const &>(*this).count());
		}
		inline bool Any() const noexcept(true)
		{
			return static_cast<Handle const &>(*this).asc();
		}
		inline const iterator_type &begin() const noexcept(true)
		{
			return std::move(static_cast<Handle const &>(*this).begin());
		}
		inline const iterator_type &end() const noexcept(true)
		{
			return std::move(static_cast<Handle const &>(*this).end());
		}

		template<typename Key>
		inline auto &operator[](Key const &key) const
		{
			return static_cast<Handle const &>(*this).operator[](key);
		}
	};
}

#endif // !IENUMERABLE_H_
