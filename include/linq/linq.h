#pragma once



#include <iostream>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>
#include <algorithm>

namespace Linq
{
	template<typename _Handle, typename Iterator>
	class iterator : public Iterator
	{
	protected:
		_Handle const &_handle;
	public:
		typedef typename Iterator::value_type value_t;


		iterator() = delete;
		iterator(iterator const &) = default;
		iterator(Iterator const rhs, _Handle const &handle)
			: Iterator(rhs), _handle(handle)
		{}

		~iterator() = default;
		
		inline value_t operator*() const { return *static_cast<Iterator const &>(*this); }
		inline value_t &operator*() { return *static_cast<Iterator &>(*this); }
		inline bool operator!=(iterator const &rhs) const
		{
			return static_cast<Iterator const &>(*this) != static_cast<Iterator const &>(rhs);
		}

		iterator &operator++()
		{
			do {
				Iterator::operator++();
			} while (!static_cast<bool>(_handle(*this)));
			return *this;
		}
	};

	//template<typename _Handle, typename Iterator>



	template<class _Handle, class _Container>
	class Object
	{
	public:
		typedef typename _Container::iterator base_iterator_t;
		typedef typename _Container::value_type value_t;
		typedef typename std::function<bool(value_t &)> lambda_t;
		typedef typename iterator<_Handle, base_iterator_t> iterator_t;
	protected:

		_Container &_container;
		lambda_t const _predicate;
		iterator_t const _begin;
		iterator_t const _end;

		Object() = delete;
		Object(Object const &) = default;
		//Object(_Container &cont, lambda_t const &pred)
		//	: _container(cont), _predicate(pred), _begin(begin), _end(end)
		//{
		//}
		//Object(_Container &cont, lambda_t const &&pred, iterator_t const begin, iterator_t const end)
		//	: _container(cont), _predicate(std::forward<lambda_t>(pred)), _begin(begin), _end(end)
		//{
		//}

		Object(_Container &cont, lambda_t const &pred, iterator_t const begin, iterator_t const end)
			: _container(cont), _predicate(pred), _begin(begin), _end(end)
		{
		}
		~Object() = default;

	public:

		bool operator()(base_iterator_t &it) const
		{
			return it == static_cast<base_iterator_t const &>(_end) || _predicate(*it);
		}

		iterator_t begin() { return _begin; }
		iterator_t end() { return _end; }

		template<typename _ContainerOut>
		_ContainerOut &to(_ContainerOut &out) const
		{
			std::copy(_begin, _end, std::back_inserter(out));
			return out;
		}

		template<typename _ContainerOut = _Container>
		_ContainerOut to() const
		{
			_ContainerOut out;
			std::copy(_begin, _end, std::back_inserter(out));
			return std::forward<_ContainerOut>(out);
		}
	};

	//template<class _Container, typename _Out_t>
	//class Select : public Object<Select<_Container, _Out_t>, _Container>
	//{

	//public:



	//	typedef typename std::vector<_Out_t>::iterator base_iterator_t;
	//	typedef typename _Container::value_type value_type;
	//	typedef typename Linq::Object<Select<std::vector<_Out_t>, _Out_t>, std::vector<_Out_t>> base_t;
	//	typedef typename Linq::iterator<Select<std::vector<_Out_t>, _Out_t>, base_iterator_t>::iterator iterator_t;

	//	typedef typename std::function<_Out_t(value_type &)> inserter_t;

	//	Select() = delete;

	//	Select(Select const &rhs)
	//		: base_t(*new std::vector<_Out_t>(rhs._container), rhs._predicate, rhs._begin, rhs._end)
	//	{
	//	}


	//	//bool operator()(base_iterator_t &it)
	//	//{
	//	//	return it == static_cast<base_iterator_t const &>(_end);
	//	//}

	//	Select(_Container &cont, inserter_t const &pred)
	//		: base_t(cont,
	//			[](value_type &) -> bool { return true; },
	//				iterator_t(_container.begin(), *this),
	//				iterator_t(_container.end()  , *this)),
	//		_inserter(pred)
	//	{
	//		//for (auto it : cont)
	//		//	_container.push_back(pred(it));
	//	}

	//	~Select() { delete &_container; }

	//	//auto where(_Pred const & next_pred) const
	//	//{
	//	//	auto pred = _predicate;
	//	//	_Pred new_pred([pred, next_pred](value_t &val) -> bool
	//	//	{
	//	//		OutType 

	//	//		return pred(val) && next_pred(val);
	//	//	});

	//	//	return Where<std::vector<Out_t>, decltype(new_pred)>(_container, new_pred);
	//	//}
	//private:
	//	inserter_t const _inserter;
	//};


	template<class _Container>
	class Where : public Object<Where<_Container>, _Container>
	{
	public:

		typedef typename _Container::iterator base_iterator_t;
		typedef typename _Container::value_type value_type;

		typedef typename Linq::Object<Where<_Container>, _Container> base_t;
		typedef typename Linq::iterator<Where<_Container>, base_iterator_t> iterator_t;

		typedef typename std::function<bool(value_type &)> lambda_t;

		Where() = delete;
		Where(Where const &rhs)
			: base_t(rhs._container, rhs._predicate, rhs._begin, rhs._end)
		{
		}

		Where(_Container &cont, lambda_t const &pred)
			: base_t(cont, pred,
				iterator_t(std::find_if(cont.begin(), cont.end(), pred), *this),
				iterator_t(cont.end(), *this))
		{
		}

		template<typename OutType>
		auto select(std::function<OutType(value_t &)> const &next_pred)
		{
			return Select<Where<_Container>, OutType>(*this, next_pred);
		}

		auto where(lambda_t const & next_pred) const
		{
			auto pred = _predicate;
			lambda_t new_pred([pred, next_pred](value_type &val) -> bool
			{
				return pred(val) && next_pred(val);
			});

			return Where<_Container>(_container, new_pred);
		}

	};
}

//template<class _Container, class _Pred, typename _Out_t>
//Linq::Select<_Container, _Pred, _Out_t> select(_Container &cont, _Pred const &&pr)
//{
//	return Linq::Select<_Container, _Pred>(cont, std::forward<_Pred const>(pr));
//}

template<class _Container, class _Pred>
Linq::Where<_Container> where(_Container &cont, _Pred const &&pr)
{
	return Linq::Where<_Container>(cont, std::forward<_Pred const>(pr));
}

template<class _Container, class _Pred>
Linq::Where<_Container> where(_Container &cont, _Pred const &pr)
{
	return Linq::Where<_Container>(cont, std::function<bool(_Container::reference)>(pr));
}