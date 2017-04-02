#ifndef ITERATOR_H_
# define ITERATOR_H_

namespace linq
{ 
	template<typename Proxy, typename Base, typename Out, iterator_type ItType>
	class iterator;

	template<typename Proxy, typename Base, typename Out>
	class iterator<Proxy, Base, Out, iterator_type::full> : public Base
	{
		Proxy const &proxy_;

	public:
		iterator() = delete;
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(Base const &IState, Proxy const &proxy)
			: Base(IState), proxy_(proxy)
		{}

		iterator const &operator++() noexcept
		{
			do
			{
				static_cast<Base &>(*this).operator++();
			} while (proxy_.validate(static_cast<Base const &>(*this)));
			return (*this);
		}
		inline Out operator*() const noexcept
		{
			return proxy_.load(static_cast<Base const &>(*this));
		}
		iterator const &operator=(iterator const &rhs)
		{
			static_cast<Base &>(*this) = static_cast<Base const &>(rhs);
			return (*this);
		}
		inline bool operator!=(iterator const &rhs) const noexcept {
			return static_cast<Base const &>(*this) != static_cast<Base const &>(rhs);
		}

	};

	template<typename Proxy, typename Base, typename Out>
	class iterator<Proxy, Base, Out, iterator_type::load> : public Base
	{
		Proxy const &proxy_;
	public:
		iterator() = delete;
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(Base const &IState, Proxy const &proxy)
			: Base(IState), proxy_(proxy)
		{}

		inline Out operator*() const noexcept { return proxy_.load(static_cast<Base const &>(*this)); }
		inline iterator const &operator++() noexcept {
			static_cast<Base &>(*this).operator++();
			return *this;
		}
		iterator const &operator=(iterator const &rhs)
		{
			static_cast<Base &>(*this) = static_cast<Base const &>(rhs);
			return (*this);
		}
		inline bool operator!=(iterator const &rhs) const noexcept {
			return static_cast<Base const &>(*this) != static_cast<Base const &>(rhs);
		}

	};

	template<typename Proxy, typename Base, typename Out>
	class iterator<Proxy, Base, Out, iterator_type::filter> : public Base
	{
		Proxy const &proxy_;
	public:
		iterator() = delete;
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(Base const &IState, Proxy const &proxy)
			: Base(IState), proxy_(proxy)
		{}

		inline Out operator*() const noexcept { return *static_cast<Base const &>(*this); }
		iterator const &operator++() noexcept {
			do
			{
				static_cast<Base &>(*this).operator++();
			} while (proxy_.validate(static_cast<Base const &>(*this)));
			return (*this);
		}
		iterator const &operator=(iterator const &rhs)
		{
			static_cast<Base &>(*this) = static_cast<Base const &>(rhs);
			return (*this);
		}
		inline bool operator!=(iterator const &rhs) const noexcept {
			return static_cast<Base const &>(*this) != static_cast<Base const &>(rhs);
		}
	};

	template<typename Proxy, typename Base, typename Out>
	class iterator<Proxy, Base, Out, iterator_type::reach> : public Base
	{
		Proxy const &proxy_;
	public:
		iterator() = delete;
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(Base const &IState, Proxy const &proxy)
			: Base(IState), proxy_(proxy)
		{}

		inline Out operator*() const noexcept { return *static_cast<Base const &>(*this); }
		iterator const &operator=(iterator const &rhs)
		{
			static_cast<Base &>(*this) = static_cast<Base const &>(rhs);
			return (*this);
		}
		inline bool operator!=(iterator const &rhs) const noexcept {
			return  static_cast<Base const &>(*this) != static_cast<Base const &>(rhs) && proxy_.reach(static_cast<Base const &>(*this));
		}
		iterator const &operator++() noexcept {
			proxy_.incr(static_cast<Base const &>(*this));
			static_cast<Base &>(*this).operator++();
			return (*this);
		}

	};

	template<typename Proxy, typename Base, typename Out>
	class iterator<Proxy, Base, Out, iterator_type::basic> : public Base
	{
	public:
		iterator() = delete;
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(Base const &IState, Proxy const &)
			: Base(IState)
		{}

		inline Out operator*() const noexcept { return *static_cast<Base const &>(*this); }
		inline iterator const &operator++() noexcept {
			static_cast<Base &>(*this).operator++();
			return *this;
		}
		iterator const &operator=(iterator const &rhs)
		{
			static_cast<Base &>(*this) = static_cast<Base const &>(rhs);
			return (*this);
		}
		inline bool operator!=(iterator const &rhs) const noexcept {
			return static_cast<Base const &>(*this) != static_cast<Base const &>(rhs);
		}
	};
}

#endif // !ITERATOR_H_
