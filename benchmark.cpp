#include <cstdlib>
#include <iostream>
#include <ctime>

#include "linq.h"
#include "assert.h"

struct heavy {
	int map[1024];

	heavy() {
		for (auto& x : map)
			x = (int)(std::rand() * 1000.0 / RAND_MAX);
	}
};

struct light {
	int map[1];

	light() {
		for (auto& x : map)
			x = (int)(std::rand() * 1000.0 / RAND_MAX);
	}
};

struct user
{
	int groupId;
	int created;

	user() {
		groupId = (int)(std::rand() * 1000.0 / RAND_MAX);
		created = (int)(std::rand() * 10.0 / RAND_MAX);
	}
};

template <typename T>
void bench()
{
	std::vector<T> cont(200000);
	auto const &data = cont;

	// benchmark tests

	std::cout << "Select.Sum" << std::endl;
	auto x1 = test("->IEnumerable", [&]() {
		return linq::make_enumerable(data)
			.Select([](const auto &val) noexcept -> const auto & { return val.map[0];; })
			.Sum();
	});
	auto x2 = test("->Legacy", [&]() {
		int result = 0;
		for (const auto &it : data)
			result += it.map[0];
		return result;
	});
	assertEquals(x1, x2);

	std::cout << "Select.Where.Sum" << std::endl;
	auto x3 = test("->IEnumerable", [&]() {
		return linq::make_enumerable(data)
			.Select([](const auto &val) noexcept -> const auto & { return val.map[0];; })
			.Where([](const auto &val) noexcept { return val > 5; })
			.Sum();
	});
	auto x4 = test("->Legacy", [&]() {
		int result = 0;
		for (const auto &it : data)
		{
			const auto val = it.map[0];
			if (val > 5)
				result += val;
		}
		return result;
	});
	assertEquals(x3, x4);

	std::cout << "Select.Skip.Take.Take.Skip.Sum" << std::endl;
	auto x5 = test("->IEnumerable", [&]() {
		return linq::make_enumerable(data)
			.Select([](const auto &val) noexcept -> const auto & { return val.map[0]; })
			.Skip(20000)
			.Take(500000)
			.Take(190000)
			.Skip(20000)
			.Sum();
	});
	auto x6 = test("->Legacy", [&]() {
		int result = 0;
		auto it = data.cbegin();
		for (int i = 0; i < 40000; ++i, ++it);
		for (int i = 0; i < 160000 && it != data.cend(); ++i, ++it)
		{
			const auto val = it->map[0];
			result += val;
		}
		return result;
	});
	auto x7 = test("->IEnumerable (Optimal Pattern)", [&]() {
		return linq::make_enumerable(data)
			.Select([](const auto &val) noexcept -> const auto & { return val.map[0]; })
			.Skip(40000)
			.Take(160000)
			.Sum();
	});
	assertEquals(x5, x6);
	assertEquals(x5, x7);

	std::cout << "Select.Skip.Take.Take.Skip.Where.Sum" << std::endl;
	auto x8 = test("->IEnumerable", [&]() {
		return linq::make_enumerable(data)
			.Select([](const auto &val) noexcept -> const auto  { return val.map[0]; })
			.Skip(20000)
			.Take(500000)
			.Take(190000)
			.Skip(20000)
			.Where([](const auto &val) noexcept { return val > 5; })
			.Sum();
	});
	auto x9 = test("->Legacy", [&]() {
		int result = 0;
		auto it = data.cbegin();
		for (int i = 0; i < 40000; ++i, ++it);
		for (int i = 0; i < 160000; ++i, ++it)
		{
			const auto val = it->map[0];
			if (val > 5)
				result += val;
		}
		return result;
	});
	auto x10 = test("->IEnumerable (Optimal Pattern)", [&]() {
		return linq::make_enumerable(data)
			.Select([](const auto &val) noexcept -> const auto  { return val.map[0]; })
			.Skip(40000)
			.Take(160000)
			.Where([](const auto &val) noexcept { return val > 5; })
			.Sum();
	});
	assertEquals(x8, x9);
	assertEquals(x8, x10);

	std::cout << "Select.Where.Skip.Take.Take.Skip.Sum" << std::endl;
	auto x11 = test("->IEnumerable", [&]() {
		return linq::make_enumerable(data)
			.Select([](const auto &val) noexcept -> const auto  { return val.map[0]; })
			.Where([](const auto &val) noexcept { return val > 5; })
			.Skip(20000)
			.Take(500000)
			.Take(190000)
			.Skip(20000)
			.Sum();
	});
	auto x12 = test("->Legacy", [&]() {
		int result = 0;
		const int limit = 190000;
		const int skip = 40000;
		int i = 0;
		int took = 0;
		for (auto it = data.cbegin(); took < limit && it != data.cend(); ++it)
		{
			const auto val = it->map[0];
			if (val > 5)
			{
				if (i >= skip)
				{
					++took;
					result += val;
				}
				++i;
			}
		}
		return result;
	});
	auto x13 = test("->IEnumerable (Optimal Pattern)", [&]() {
		return linq::make_enumerable(data)
			.Select([](const auto &val) noexcept -> const auto  { return val.map[0]; })
			.Where([](const auto &val) noexcept { return val > 5; })
			.Skip(40000)
			.Take(160000)
			.Sum();
	});
	assertEquals(x11, x12);
	assertEquals(x11, x13);
	std::cout << std::endl;

	auto x16 = test("StressTest->IEnumerable (GroupBy)", [&]() {
		return linq::make_enumerable(data)
			.Select([](const auto &val) noexcept -> const auto  { return val.map[0]; })
			.Where([](const auto &val) noexcept { return val > 5; })
			.Skip(40000)
			.Take(160000)
			.GroupBy([](const auto &key) -> auto { return key; }, [](const auto &key) -> auto { return key; })
			.Select([](auto &pair) {
			return linq::make_enumerable(pair.second[pair.first])
				.Sum(); })
			.Sum();
	});

	//auto x17 = test("StressTest->IEnumerable (GroupBy.ThenBy)", [&]() {
	//	return linq::make_enumerable(data)
	//		.Select([](const auto &val) noexcept -> const auto { return val.map[0]; })
	//		.Where([](const auto &val) noexcept { return val > 5; })
	//		.Skip(40000)
	//		.Take(160000)
	//		.GroupBy([](const auto &key) -> const auto { return key; })
	//		//.ThenBy([](const auto &key) -> const auto { return key; })
	//		.Select([](const auto &pair) { return linq::make_enumerable(pair.second).Sum(); })
	//		.Sum();
	//});

	//assertEquals(x13, x16);
	//assertEquals(x13, x17);

}

#include <unordered_map>

template <>
void bench<user>()
{
	std::vector<user> data(200000);

	auto x0 = test("->IEnumerable (GroupBy)", [&]() {
		return linq::make_enumerable(data)
			.Where([](const auto &val) noexcept { return val.groupId > 5; })
			.GroupBy(
				[](const auto &key) noexcept { return key.groupId; },
				[](const auto &key) { return key.created; });
	});

	std::unordered_map<int, std::unordered_map<int, std::vector<user>>> group;
	auto x1 = test("->Legacy (GroupBy)", [&]() {
		for (const auto &it : data)
			if (it.groupId > 5)
				group[it.groupId][it.created].push_back(it);
		return 0;
	});

	auto x2 = test("->IEnumerable (OrderBy)", [&]() {
		return linq::make_enumerable(data)
			.Where([](const auto &val) noexcept { return val.groupId > 5; })
			.OrderBy(
				[](const auto &key) noexcept { return key.groupId; },
				[](const auto &key) { return key.created; }).Desc();
	});

	std::cout << std::endl;
}

int main(int argc, char *argv[])
{
	std::srand(time(0));
	//std::cout << "# Light objects" << std::endl;
	//bench<light>();

	//std::cout << "# Heavy objects" << std::endl;
	//bench<heavy>();

	std::cout << "# User objects" << std::endl;
	bench<user>();

	return EXIT_SUCCESS;
}
