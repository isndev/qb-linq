#include <cstdlib>
#include <iostream>
#include <ctime>

#include "linq.h"
#include "assert.h"

struct light
{
	int a;
	light(const light &) = default;
	light()
		: a(std::rand())
	{}

};

struct heavy
{
	int a;
	int map[1020];
	heavy(const heavy &) = default;
	heavy()
		: a(std::rand())
	{}

};

template <typename T>
void bench()
{
	std::vector<T> data(200000);

	// test de routine

	//auto x0 = test("legacy_copy", [&]() {
	//	return data;
	//});

	//auto x1 = test("from_select", [&]() {
	//	return std::move(linq::from(data)
	//		.select([](auto &val) -> int & { return val.a; })
	//		.to<std::vector<int>>());
	//});

	//auto x2 = test("from_where", [&]() {
	//	return std::move(linq::from(data)
	//		.where([](auto &val) { return val.a < 10; })
	//		.to<std::vector<T>>());
	//});

	//auto x3 = test("from_where_where", [&]() {
	//	return std::move(linq::from(data)
	//		.where([](auto &val) { return val.a < 1000; })
	//		.where([](auto &val) { return val.a == 10; })
	//		.to<std::vector<T>>());
	//});

	//auto x4 = test("from_select_where", [&]() {
	//	return std::move(linq::from(data)
	//		.select([](auto &val) -> auto & { return val.a; })
	//		.where([](auto &val) { return val < 10; })
	//		.to<std::vector<int>>());
	//});

	//auto x5 = test("from_select_where", [&]() {
	//	return std::move(linq::from(data)
	//		.select([](auto val) { return val.a; })
	//		.where([](auto val) { return val < 10; })
	//		.to<std::vector<int>>());
	//});

	//auto x5 = test("from_select_where_where", [&]() {
	//	return std::move(linq::from(data)
	//		.select<int>([](auto &val) { return val.a; })
	//		.where([](auto &val) { return val < 1000; })
	//		.where([](auto &val) { return val == 10; })
	//		.to<std::vector<int>>());
	//});

	// benchmark tests

	//auto x6 = test(" from_select_sum", [&]() {
	//	return linq::from(data)
	//		.select([](const auto val) noexcept { return val.a; })
	//		.sum();
	//});

	//auto x7 = test("&from_select_sum", [&]() {
	//	return linq::from(data)
	//		.select([](const auto &val) noexcept -> const auto & { return val.a; })
	//		.sum();
	//});

	//auto x8 = test("legacy_select_sum", [&]() {
	//	int result = 0;
	//	for (const auto &it : data)
	//		result += it.a;
	//	return result;
	//});

	//assertEquals(x6, x7);
	//assertEquals(x6, x8);

	//auto x9 = test(" from_select_where_sum", [&]() {
	//	return linq::from(data)
	//		.select([](const auto val) noexcept { return val.a; })
	//		.where([](const auto val) noexcept { return val < 1000; })
	//		.sum();
	//});

	auto x10 = test("&from_select_where_sum", [&]() {
		return linq::from(data)
			.select([](const auto &val) noexcept -> const auto & { return val.a; })
			.where([](const auto &val) noexcept { return val < 1000; })
			.sum();
	});

	auto x11 = test("legacy_select_where_sum", [&]() {
		int result = 0;
		for (const auto &it : data)
			if (it.a < 1000)
				result += it.a;
		return result;
	});

	//assertEquals(x9, x10);
	//assertEquals(x9, x11);

}

int main(int argc, char *argv[])
{
	std::srand(time(0));

	std::cout << "# Light objects" << std::endl;
	bench<light>();

	std::cout << "# Heavy objects" << std::endl;
	bench<heavy>();

	return EXIT_SUCCESS;
}