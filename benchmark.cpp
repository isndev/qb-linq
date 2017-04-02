#include <cstdlib>
#include <iostream>
#include <ctime>

#include "linq.h"
#include "assert.h"

struct heavy {
	int map[1024];

	heavy() {
		for (auto& x : map)
			x = (int)(std::rand() * 10000.0 / RAND_MAX);
	}
};

struct light {
	int map[1];

	light() {
		for (auto& x : map)
			x = (int)(std::rand() * 10000.0 / RAND_MAX);
	}
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
	//		.select([](auto &val) -> int & { return val.map[0]; })
	//		.to<std::vector<int>>());
	//});

	//auto x2 = test("from_where", [&]() {
	//	return std::move(linq::from(data)
	//		.where([](auto &val) { return val.map[0] < 10; })
	//		.to<std::vector<T>>());
	//});

	//auto x3 = test("from_where_where", [&]() {
	//	return std::move(linq::from(data)
	//		.where([](auto &val) { return val.map[0] < 1000; })
	//		.where([](auto &val) { return val.map[0] == 10; })
	//		.to<std::vector<T>>());
	//});

	//auto x4 = test("from_select_where", [&]() {
	//	return std::move(linq::from(data)
	//		.select([](auto &val) -> auto & { return val.map[0]; })
	//		.where([](auto &val) { return val < 10; })
	//		.to<std::vector<int>>());
	//});

	//auto x5 = test("from_select_where", [&]() {
	//	return std::move(linq::from(data)
	//		.select([](auto val) { return val.map[0]; })
	//		.where([](auto val) { return val < 10; })
	//		.to<std::vector<int>>());
	//});

	//auto x5 = test("from_select_where_where", [&]() {
	//	return std::move(linq::from(data)
	//		.select<int>([](auto &val) { return val.map[0]; })
	//		.where([](auto &val) { return val < 1000; })
	//		.where([](auto &val) { return val == 10; })
	//		.to<std::vector<int>>());
	//});

	//auto x11 = test("from", [&]() {
	//	const auto &obj = linq::from(data)
	//		.select([](const auto &val) noexcept -> const auto & { return val.map[0]; })
	//		.where([](const auto &val) noexcept { return val < 1000; });
	//	return linq::from(obj)
	//		.where([](const auto &val) noexcept { return val < 500; })
	//		.select([](const auto &val) noexcept -> double { return static_cast<double>(val / 3.); })
	//		.where([](const auto val) noexcept { return val < 10; })
	//		.select([](const auto val) noexcept -> int { return static_cast<int>(val / 3.); })
	//		.to<std::vector<int>>();
	//});


	// benchmark tests

	auto x6 = test(" from_select_sum", [&]() {
		return linq::from(data)
			.select([](const auto val) noexcept { return val.map[0];; })
			.sum();
	});

	auto x7 = test("&from_select_sum", [&]() {
		return linq::from(data)
			.select([](const auto &val) noexcept -> const auto & { return val.map[0];; })
			.sum();
	});

	auto x8 = test("legacy_select_sum", [&]() {
		int result = 0;
		for (const auto &it : data)
			result += it.map[0];
		return result;
	});

	assertEquals(x6, x7);
	assertEquals(x6, x8);

	auto x9 = test(" from_select_where_sum", [&]() {
		return linq::from(data)
			.select([](const auto &val) noexcept -> const auto { return val.map[0]; })
			.where([](const auto val) noexcept { return val > 5; })
			.sum();
	});

	auto x10 = test("&from_select_where_sum", [&]() {
		return linq::from(data)
			.select([](const auto &val) noexcept -> const auto & { return val.map[0]; })
			.where([](const auto &val) noexcept { return val > 5; })
			.sum();
	});

	auto x12 = test("legacy_select_where_sum", [&]() {
		int result = 0;
		for (const auto it : data)
			if (it.map[0] > 5)
				result += it.map[0];
		return result;
	});

	//assertEquals(x9, x10);
	//assertEquals(x9, x11);


	auto x1 = test("from_select", [&]() {
		return std::move(linq::from(data)
			.select([](auto const &val) { return val.map[0]; })
			.where([](auto const val) { return val < 1000; })
			.skip(10)
			.take(10)
			//.count());
//			.max());
			.groupBy([](const auto key) -> int { return key;}));
			//.to<std::vector<int>>());
	});

}

int main(int argc, char *argv[])
{
	std::srand(time(0));

	std::cout << "# Light objects" << std::endl;
	bench<light>();

	//std::cout << "# Heavy objects" << std::endl;
	//bench<heavy>();

	return EXIT_SUCCESS;
}