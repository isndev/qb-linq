#pragma once



#include <iostream>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>
#include <algorithm>

namespace Linq
{
	template<class _ContType, class _Pred>
	class Where
	{
	public:
		using Type = Where<_ContType, _Pred>;

		typedef typename _ContType::iterator iterator_type;
		typedef typename _ContType::value_type value_type;
		typedef typename _ContType::const_iterator const_iterator_type;
		class where_iterator : public iterator_type
		{
		private:
			iterator_type const &_end;

			Where<_ContType, _Pred> const &_context;
		public:
			where_iterator() = delete;
			where_iterator(where_iterator const &rhs)
				: iterator_type(static_cast<iterator_type>(rhs)), _end(rhs._end), _context(rhs._context)
			{
			}

			where_iterator(iterator_type const &rhs, iterator_type const &end, Where<_ContType, _Pred> const &ctxt)
				: iterator_type(rhs), _end(end), _context(ctxt)
			{
			}

			where_iterator(where_iterator const &rhs, Where<_ContType, _Pred> const &ctxt)
				: iterator_type(rhs), _end(rhs._end), _context(ctxt)
			{}

			where_iterator &operator++()
			{
				iterator_type::operator++();
				while (static_cast<iterator_type>(*this) != _end && !_context._pred(*static_cast<iterator_type>(*this)))
					iterator_type::operator++();
				return (*this);
			}

			where_iterator operator++(int)
			{
				auto itmp = *this;
				return ++itmp;
			}

			value_type operator*()
			{
				return *static_cast<iterator_type>(*this);
			}
		};
		//typedef std::function_tr  decltype(functor) lambda_type;


		Where() = delete;
		Where(Type const &rhs)
			: _pred(rhs._pred), _container(rhs._container), _end(rhs._end), _begin(rhs._begin)
		{

		}
		Where(_ContType &_cont, _Pred const &_pr)
			: _pred(_pr), _container(_cont), _end(_cont.end(), _cont.end(), *this), _begin(best_start(_cont.begin()), _cont.end(), *this)
		{}

		where_iterator begin() const
		{
			return _begin;
		}
		where_iterator end() const { return _end; }

		template<class _PredRec>
		auto where(_PredRec const & next_pred) const
		{
			auto &pred = _pred;
			auto new_pred = [pred, &next_pred](value_type &val) -> bool
			{
				return pred(val) && next_pred(val);
			};

			return Where<_ContType, decltype(new_pred)>(_container, new_pred);
		}

		//template <typename T>
		//to()
		//{
		//	
		//}

	private:
		friend class where_iterator;

		_Pred const &_pred;
		_ContType &_container;
		where_iterator const _end;
		where_iterator const _begin;

		iterator_type const best_start(iterator_type begin) const
		{
			while (begin != _end && !_pred(*begin))
				++begin;
			return begin;
		}

	};
}


//template<typename &_ContainerType, typename _Pred>
//class ILinq
//{
//	_ContainerType _container;
//	_Pred _predicate;
//public:
//	template<typename T, typename From>
//	void to()
//	{
//		//for (auto it : _container)
//	}
//};
//
//template<class &_ContainerType>
//class From
//{
//
//};
//
//template<typename T, class Container>
//class Select
//{
//	Container const &_container;
//public:
//
//	Select(Container const &, )
//
//	template<typename 
//
//	template<typename T>
//	void select<>
//
//};



// from().select()
template<class _ContType, class _Pred>
Linq::Where<_ContType, _Pred> where(_ContType &cont, _Pred const &pr)
{
	return Linq::Where<_ContType, _Pred>(cont, pr);
}