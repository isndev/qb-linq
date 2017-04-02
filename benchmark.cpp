

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

		//class const_iterator : public _ContType::const_iterator
		//{
		//public:
		//};


		//static const auto functor = [](_ContType::const_reference) -> bool {};

	public:
		typedef typename _ContType::iterator iterator_type;
		class iterator : public iterator_type
		{
		public:
			iterator(iterator_type const &rhs)
				: iterator_type(rhs)
			{}

			iterator_type &operator++()
			{
				return iterator_type::operator++();
			}
		};
		//typedef std::function_tr  decltype(functor) lambda_type;
		

		Where() = delete;

		Where(_ContType &_cont, _Pred const _pr)
			: _container(_cont), _pred(_pr), _begin(_cont.begin()), _end(_cont.end())
		{
		}

		iterator begin() { return _begin; }
		iterator end() { return _end; }

		void run()
		{
			int a = 120;
			_pred(a);
		}

	private:
		_ContType &_container;
		_Pred const _pred;

		iterator _begin;
		iterator _end;
		//std::function<bool(ElementType)>  _validator;

	};
}

template<class _ContType, class _Pred>
Linq::Where<_ContType, _Pred> where(_ContType cont, _Pred pr)
{
	return Linq::Where<_ContType, _Pred>(cont, pr);
}



int main(int argc, char *argv[])
{
	std::vector<int> vec = { 1 , 1 , 2 , 3 , 5 };
	auto bite = [](int &a) { return a < 3; };
	int i = 0;

	auto where1 = where(vec, bite);
	auto where2 = where(vec, [](int &a) { return a < 3; });
	auto where3 = where(vec, [i](int &a) { 
		std::cout << i << std::endl;
		return a < 3; });
	//where3.run();
	//auto mywhere = Linq::Where<>(vec, bite);

	return EXIT_SUCCESS;
}