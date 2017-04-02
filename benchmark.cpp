#include <cstdlib>
#include <iostream>
#include <ctime>

#include "linq.h"
#include "assert.h"

struct light
{
	int a;
	light(const light &) = default;
	//light const &operator=(light const &rhs)
	//{
	//	a = rhs.a;
	//	return *this;
	//}
	light()
		: a(std::rand())
	{}

};

struct heavy
{
	int a;
	int map[1020];
	heavy(const heavy &) = default;
	//heavy const &operator=(heavy const &rhs)
	//{
	//	a = rhs.a;
	//	return *this;
	//}
	heavy()
		: a(std::rand())
	{}

};

template <typename T>
void bench()
{
	std::vector<T> data(200000);

	auto x0 = test("legacy_copy", [&]() {
		return data;
	});

	auto x1 = test("from_select", [&]() {
		return std::move(linq::from(data)
			.select<int>([](auto &val) { return val.a; })
			.to<std::vector<int>>());
	});

	auto x2 = test("from_where", [&]() {
		return std::move(linq::from(data)
			.where([](auto &val) { return val.a < 10; })
			.to<std::vector<T>>());
	});

	auto x3 = test("from_where_where", [&]() {
		return std::move(linq::from(data)
			.where([](auto &val) { return val.a < 1000; })
			.where([](auto &val) { return val.a == 10; })
			.to<std::vector<T>>());
	});

	auto x4 = test("from_select_where", [&]() {
		return std::move(linq::from(data)
			.select<int>([](auto &val) { return val.a; })
			.where([](auto &val) { return val < 10; })
			.to<std::vector<int>>());
	});

	auto x5 = test("from_select_where_where", [&]() {
		return std::move(linq::from(data)
			.select<int>([](auto &val) { return val.a; })
			.where([](auto &val) { return val < 1000; })
			.where([](auto &val) { return val == 10; })
			.to<std::vector<int>>());
	});


}

int main(int argc, char *argv[])
{
	std::srand(time(0));

	std::cout << "# Light objects" << std::endl;
	bench<light>();

	std::cout << "# Heavy objects" << std::endl;
	bench<heavy>();
	//for (auto it : select)
	//{
	//	std::cout << it << std::endl;
	//}


	return EXIT_SUCCESS;
}