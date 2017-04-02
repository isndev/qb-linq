#include "Header.h"

//template<class _ContType, class _Pred>
//Linq::Select<_ContType, _Pred> select(_ContType &cont, _Pred &&pr)
//{
//	return Linq::Where<_ContType, _Pred>(cont, std::forward<_Pred>(pr));
//}

bool test_test(int &a)
{
	a += 10;
	return false;
}


class A
{
public:
	int a;
public:
	A() = default;
	~A()
	{
		a;
	}
};

//from Container .select.where.where.select.toArray.select

int main(int argc, char *argv[])
{
	std::vector<int>::iterator it;

	std::vector<int> vec = { 2, 1 , 2 , 1 , 2 , 1, 3, 5, 1, 2 };
	std::vector<double> vec2 = { 1., 2., 3.};
	int i = 4;
	auto bite = [i](int  &a) { return a == 1; };
	auto poil = [i](int  &a) { return a < i; };
	//auto where1 = where(vec, &test_test);
	auto where2 = where(vec, [i](int  &a) { return a < i; });
	auto where3 = where2.where([i](int  &a) { return a <= 2; })
						.where([i](int &val) {return val == i-3;  });// .to<std::vector<int>>(vec2);
	//auto where4 = where3;
	for (auto it : where3)
	{
		std::cout << it << std::endl;
	}

	auto where_d = where(vec2, [](double  &a) { return a <= 3.; }).where([](double  &a) { return a == 1.; });

	for (auto it : where_d)
	{
		std::cout << it << std::endl;
	}

	//auto select_s = where3.select<double>([] (int &value) -> double {
	//	return value / 2;
	//});

	//for (auto &&a : where3)
	//{
	//	std::cout << a;
	//}

	//where3.run();
	//auto mywhere = Linq::Where<>(vec, bite);

	return EXIT_SUCCESS;
}

