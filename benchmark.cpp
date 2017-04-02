#include "linq/linq.h"

bool test_test(int a)
{
	a += 10;
	return true;
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

	std::vector<int> vec = { 1 , 2 , 1 , 2 , 1, 3, 5, 1, 2 };
	std::vector<int> vec2 = { 1 };
	int i = 3;
	auto const bite = [i](int  &a) { return a == 1; };
	auto const poil = [i](int  &a) { return a < i; };
	//auto where1 = where(vec, &test_test);
	auto where2 = where(vec, poil);
	auto where3 = where2.where(bite);
	auto where4 = where3;
	for (auto it = where3.begin(); it != where3.end(); ++it)
	{
		std::cout << *it << std::endl;
	}

	//for (auto &&a : where3)
	//{
	//	std::cout << a;
	//}

	//where3.run();
	//auto mywhere = Linq::Where<>(vec, bite);

	return EXIT_SUCCESS;
}

