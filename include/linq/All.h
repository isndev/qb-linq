#ifndef ALL_H_
# define ALL_H_

namespace linq
{
	template <typename Base, typename Proxy>
	class all_it : public Base {
	public:
		typedef Base iterator;
		typedef decltype(*std::declval<Base>()) out;

		all_it() = delete;
		all_it(all_it const &) = default;
		all_it(Base const &base, Proxy proxy) noexcept(true)
			: Base(base), _proxy(proxy) {}

		auto const &operator=(all_it const &rhs) noexcept(true) {
			static_cast<Base>(*this) = static_cast<Base const &>(rhs);
		}
	
	private:
		Proxy _proxy;
	};

	template<typename BaseIt, typename Proxy>
	class All : public IState<all_it<BaseIt, Proxy>>{
	public:
		typedef all_it<BaseIt, Proxy> iterator;
		typedef iterator const_iterator;

		using base_t = IState<iterator>;
	public:
		~All() = default;
		All() = delete;
		All(All const &) = default;
		All(BaseIt const &begin, BaseIt const &end, Proxy proxy)
			: base_t(iterator(begin, proxy), iterator(end, proxy)), proxy_(proxy) {}

		constexpr auto asc() const noexcept(true) { return *this; }
		constexpr auto desc() const noexcept(true) {
			return All<decltype(proxy_->rbegin()), Proxy>(proxy_->rbegin(), proxy_->rend(), proxy_);
		}

		template<typename Key>
		constexpr auto &operator[](Key const &key) const {
			return (*proxy_).at(key);
		}

	private:
		Proxy proxy_;
	};
}

#endif // !All_H_
