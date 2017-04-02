#ifndef IENUMERABLE_H_
# define IENUMERABLE_H_

namespace linq
{
	template <typename Handle>
	class IEnumerable : public Handle
	{
		typedef typename Handle::iterator iterator_t;
		typedef decltype(*std::declval<iterator_t>()) out_t;

	public:
		IEnumerable() = delete;
		~IEnumerable() = default;
		IEnumerable(IEnumerable const &) = default;
		IEnumerable(Handle const &rhs)
			: Handle(rhs) {}

		template<typename Func>
		inline auto Select(Func const &next_loader) const noexcept
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).select(next_loader))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).select(next_loader)));
		}
		template<typename... Funcs>
		inline auto SelectMany(Funcs const &...keys) const noexcept
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).selectMany(keys...))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).selectMany(keys...)));
		}
		template<typename Func>
		inline auto Where(Func const &next_filter) const noexcept
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).where(next_filter))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).where(next_filter)));
		}
		template<typename... Funcs>
		inline auto GroupBy(Funcs const &...keys) const noexcept
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).groupBy(keys...))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).groupBy(keys...)));
		}
		template<typename... Funcs>
		inline auto OrderBy(Funcs const &...keys) const noexcept
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).orderBy(keys...))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).orderBy(keys...)));
		}
		template<typename Func>
		inline auto SkipWhile(Func const &func) const noexcept
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).skipWhile(func))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).skipWhile(func)));
		}
		// TakeWhile todo

		inline auto Asc()
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).asc())>;
			return ret_t(std::move(static_cast<Handle const &>(*this).asc()));
		}
		inline auto Desc()
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).desc())>;
			return ret_t(std::move(static_cast<Handle const &>(*this).desc()));
		}

		inline auto Skip(std::size_t offset) const noexcept
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).skip(offset))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).skip(offset)));
		}
		inline auto Take(std::size_t limit) const noexcept
		{
			using ret_t = IEnumerable<decltype(static_cast<Handle const &>(*this).take(limit))>;
			return ret_t(std::move(static_cast<Handle const &>(*this).take(limit)));
		}
		inline auto Min() const noexcept
		{
			return std::move(static_cast<Handle const &>(*this).min());
		}
		inline auto Max() const noexcept
		{
			return std::move(static_cast<Handle const &>(*this).max());
		}
		inline auto Sum() const noexcept
		{
			return std::move(static_cast<Handle const &>(*this).sum());
		}
		inline auto Count() const noexcept
		{
			return std::move(static_cast<Handle const &>(*this).count());
		}
		inline const iterator_t begin() const noexcept
		{
			return std::move(static_cast<Handle const &>(*this).begin());
		}
		inline const iterator_t end() const noexcept
		{
			return std::move(static_cast<Handle const &>(*this).end());
		}

		//template<typename OutProxy>
		//inline OutProxy To() const noexcept
		//{
		//	return std::move(static_cast<Handle &>(*this).to< OutProxy >());
		//}
		template<typename OutProxy>
		inline void To(OutProxy &out) const noexcept
		{
			return std::move(static_cast<Handle const &>(*this).to(out));
		}

		template<typename T>
		inline auto &operator[](T const &key) const
		{
			return static_cast<Handle const &>(*this).operator[](key);
		}

	};
}

#endif // !IENUMERABLE_H_
