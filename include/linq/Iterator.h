#ifndef ITERATOR_H_
# define ITERATOR_H_

namespace linq
{ 
	template<typename Base, typename Trait, typename Out, eIteratorType ItType>
	class iterator;

	//template<typename Base, typename Trait, typename Out>
	//class iterator<Base, Trait, Out, eIteratorType::full> : public Base, public Trait
	//{
	//public:
	//	iterator() = delete;
	//	~iterator() = default;
	//	iterator(iterator const &) = default;
	//	iterator(Base const &IState, Trait const &&trait)
	//		: Base(IState), Trait(trait)
	//	{}

	//	iterator const &operator++() noexcept
	//	{
	//		do
	//		{
	//			static_cast<Base &>(*this).operator++();
	//		} while (validate(static_cast<Base const &>(*this)));
	//		return (*this);
	//	}
	//	inline Out operator*() const noexcept
	//	{
	//		return load(static_cast<Base const &>(*this));
	//	}
	//	iterator const &operator=(iterator const &rhs)
	//	{
	//		static_cast<Trait &>(*this) = static_cast<Trait const &>(rhs);
	//		static_cast<Base &>(*this) = static_cast<Base const &>(rhs);
	//		return (*this);
	//	}
	//	inline bool operator!=(iterator const &rhs) const noexcept {
	//		return static_cast<Base const &>(*this) != static_cast<Base const &>(rhs);
	//	}

	//};

	//template<typename Trait, typename Base, typename Out>
	//class iterator<Trait, Base, Out, eIteratorType::load> : public Base
	//{
	//	Trait const &proxy_;
	//public:
	//	iterator() = delete;
	//	~iterator() = default;
	//	iterator(iterator const &) = default;
	//	iterator(Base const &IState, Trait const &proxy)
	//		: Base(IState), proxy_(proxy)
	//	{}

	//	inline Out operator*() const noexcept { return proxy_.load(static_cast<Base const &>(*this)); }
	//	inline iterator const &operator++() noexcept {
	//		static_cast<Base &>(*this).operator++();
	//		return *this;
	//	}
	//	iterator const &operator=(iterator const &rhs)
	//	{
	//		static_cast<Base &>(*this) = static_cast<Base const &>(rhs);
	//		return (*this);
	//	}
	//	inline bool operator!=(iterator const &rhs) const noexcept {
	//		return static_cast<Base const &>(*this) != static_cast<Base const &>(rhs);
	//	}

	//};

	//template<typename Trait, typename Base, typename Out>
	//class iterator<Trait, Base, Out, eIteratorType::filter> : public Base
	//{
	//	Trait const &proxy_;
	//public:
	//	iterator() = delete;
	//	~iterator() = default;
	//	iterator(iterator const &) = default;
	//	iterator(Base const &IState, Trait const &proxy)
	//		: Base(IState), proxy_(proxy)
	//	{}

	//	inline Out operator*() const noexcept { return *static_cast<Base const &>(*this); }
	//	iterator const &operator++() noexcept {
	//		do
	//		{
	//			static_cast<Base &>(*this).operator++();
	//		} while (proxy_.validate(static_cast<Base const &>(*this)));
	//		return (*this);
	//	}
	//	iterator const &operator=(iterator const &rhs)
	//	{
	//		static_cast<Base &>(*this) = static_cast<Base const &>(rhs);
	//		return (*this);
	//	}
	//	inline bool operator!=(iterator const &rhs) const noexcept {
	//		return static_cast<Base const &>(*this) != static_cast<Base const &>(rhs);
	//	}
	//};

	//template<typename Trait, typename Base, typename Out>
	//class iterator<Trait, Base, Out, eIteratorType::reach> : public Base
	//{
	//	Trait const &proxy_;
	//public:
	//	iterator() = delete;
	//	~iterator() = default;
	//	iterator(iterator const &) = default;
	//	iterator(Base const &IState, Trait const &proxy)
	//		: Base(IState), proxy_(proxy)
	//	{}

	//	inline Out operator*() const noexcept { return *static_cast<Base const &>(*this); }
	//	iterator const &operator=(iterator const &rhs)
	//	{
	//		static_cast<Base &>(*this) = static_cast<Base const &>(rhs);
	//		return (*this);
	//	}
	//	inline bool operator!=(iterator const &rhs) const noexcept {
	//		return  static_cast<Base const &>(*this) != static_cast<Base const &>(rhs) && proxy_.reach(static_cast<Base const &>(*this));
	//	}
	//	iterator const &operator++() noexcept {
	//		proxy_.incr(static_cast<Base const &>(*this));
	//		static_cast<Base &>(*this).operator++();
	//		return (*this);
	//	}

	//};

	template<typename Base, typename Trait, typename Out>
	class iterator<Base, Trait, Out, eIteratorType::basic> : public Base, public Trait
	{
	public:
		iterator() = delete;
		~iterator() = default;
		iterator(iterator const &) = default;
		iterator(Base const &base, Trait const &trait)
			: Base(base), Trait(trait)
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
