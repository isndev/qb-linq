#include <cstdlib>
#include <iostream>
#include <ctime>

#include "linq/linq.h"
#include "assert.h"

#define SEPARATOR_TEST "------------------------------------------------------"

struct User
{
	int id;
	int group;
	int category;
	int likes;
	int visits;
	User(int seed)
		: id(seed), group(seed % 1024), category(seed % 128), likes(seed % 8192), visits((seed * 2) % 16384)
	{
	}
};

struct UserRandom
{
	int id;
	int group;
	int category;
	int likes;
	int visits;
	UserRandom(int seed)
		: id(seed), group((int)(std::rand() * 1000.0 / RAND_MAX) % 1024),
		category((int)(std::rand() * 1000.0 / RAND_MAX) % 128),
		likes((int)(std::rand() * 1000.0 / RAND_MAX) % 8192),
		visits((int)(std::rand() * 1000.0 / RAND_MAX) % 16384)
	{
	}
};

enum class which
{
	From,
	Take,
	Skip,
	All,
	Select,
	Where,
	SelectMany,
	GroupBy,
	OrderBy,
	Custom

};

template <typename T>
class Flush_cache : public T
{
public:
	Flush_cache(int const magic)
		: T(magic)
	{}
};

template <typename T>
class Context
{
	const int Max;

	std::vector<Flush_cache<T>> context_;
	const std::vector<Flush_cache<T>> &context_c;

public:
	Context(int max = 200000)
		: Max(max), context_c(context_)
	{
		std::cout << SEPARATOR_TEST << std::endl;
		context_.reserve(Max);
		for (int i = 0; i < Max; ++i)
			context_.push_back({ i });
	}
	auto &get() { return context_; }
	auto const &cget() const { return context_c; }
};
template <>
class Context<int>
{
	const int Max;

	std::vector<int> context_;
	const std::vector<int> &context_c;

public:
	Context(int max = 200000)
		: Max(max), context_c(context_)
	{
		std::cout << SEPARATOR_TEST << std::endl;
		context_.reserve(Max);
		for (int i = 0; i < Max; ++i)
			context_.push_back({ i % 12345 });
	}
	auto &get() { return context_; }
	auto &cget() const { return context_c; }
};

template <typename T, which type>
struct Test;
/* Tests enum vs simple vector<int>*/
template <>
struct Test<int, which::From>
{
	auto operator()() const
	{
		Context<int> context;
		auto &data = context.get();
		return
		test("Naive->From", [&]() {
			int result = 0;
			for (const auto &it : data)
				result += it;
			return result;
		})
		==
		test("IEnum->From", [&]() {
			return linq::make_enumerable(data)
				.Sum();
		});
	}
};
template <>
struct Test<int, which::Take>
{
	auto operator()() const
	{
		Context<int> context;
		auto &data = context.get();
		return
		test("Naive->Take", [&]() {
			int result = 0;
			for (int i = 0; i < 100000; ++i)
				result += data[i];
			return result;
		})
		==
		test("IEnum->Take", [&]() {
			return linq::make_enumerable(data)
				.Take(100000)
				.Sum();
		});
	}
};
template <>
struct Test<int, which::Skip>
{
	auto operator()() const
	{
		Context<int> context;
		auto &data = context.get();
		return test("Naive->Skip", [&]() {
			int result = 0;
			auto begin = data.begin();
			for (int i = 0; i < 100000; ++i, ++begin);
			for (;begin != data.end(); ++begin)
				result += *begin;
			return result;
		})
		==
		test("IEnum->Skip", [&]() {
			return linq::make_enumerable(data)
				.Skip(100000)
				.Sum();
		});
	}
};
template <>
struct Test<int, which::Where>
{
	auto operator()() const
	{
		Context<int> context;
		auto &data = context.get();
		return test("Naive->Where", [&]() {
			int result = 0;
			for (const auto &it : data)
			{
				const auto &val = it;
				if (val > 1234)
					result += val;
			}
			return result;
		})
		==
		test("IEnum->Where", [&]() {
			return linq::make_enumerable(data)
				.Where([](const auto &val) noexcept(true) { return val > 1234; })
				.Sum();
		});
	}
};
/* Tests enum vs complexe vector<object>*/
template <typename T>
struct Test<T, which::Select>
{
	auto operator()() const
	{
		Context<T> context;
		auto &data = context.get();
		return
			test("Naive->Select", [&]() {
			int result = 0;
			for (const auto &it : data)
				result += it.id;
			return result;
		})
		==
		test("IEnum->Select", [&]() {
			return linq::make_enumerable(data)
				.Select([](const auto &val) noexcept(true) -> const auto { return val.id; })
				.Sum();
		});
	}
};
template <typename T>
struct Test<T, which::Take>
{
	auto operator()() const
	{
		Context<T> context;
		auto &data = context.get();
		return
		test("Naive->Take", [&]() {
			int result = 0;
			int i = 0;
			for (const auto &it : data)
			{
				const auto val = it.id;
				if (i < 100000)
					result += val;
				else break;
				++i;
			}
			return result;
		})
		==
		test("IEnum->Take", [&]() {
			return linq::make_enumerable(data)
				.Select([](const auto &val) noexcept(true) -> const auto { return val.id; })
				.Take(100000)
				.Sum();
		});
	}
};
template <typename T>
struct Test<T, which::Skip>
{
	auto operator()() const
	{
		Context<T> context;
		auto &data = context.get();

		return test("Naive->Skip", [&]() {
			int result = 0;
			auto begin = data.begin();
			for (int i = 0; i < 100000; ++i, ++begin);
			for (; begin != data.end(); ++begin)
				result += (*begin).id;
			return result;
		})
		==
		test("IEnum->Skip", [&]() {
			return linq::make_enumerable(data)
				.Select([](const auto &val) noexcept(true) -> const auto { return val.id; })
				.Skip(100000)
				.Sum();
		});
	}
};
template <typename T>
struct Test<T, which::Where>
{
	auto operator()() const
	{
		Context<T> context;
		auto &data = context.get();
		return test("Naive->Where", [&]() {
			int result = 0;
			for (const auto &it : data)
			{
				const auto &val = it.id;
				if (val > 1234)
					result += val;
			}
			return result;
		})
		==
		test("IEnum->Where", [&]() {
			return linq::make_enumerable(data)
				.Select([](const auto &val) noexcept(true) -> const auto & { return val.id; })
				.Where([](const auto &val) noexcept(true) { return val > 1234; })
				.Sum();
		});
	}
};
template <typename T>
struct Test<T, which::SelectMany>
{
	auto operator()() const
	{
		Context<T> context;
		auto &data = context.get();
		return test("Naive->SelectM", [&]() {
			int result = 0;
			for (const auto &it : data)
			{
				std::tuple<int, int, int, int> tupl(it.group, it.category, it.likes, it.visits);
				result += std::get<0>(tupl);
			}
			return result;
		})
		==
		test("IEnum->SelectM", [&]() {
			return linq::make_enumerable(data)
				.SelectMany(
					[](const auto &key) noexcept(true) { return key.group; },
					[](const auto &key) noexcept(true) { return key.category; },
					[](const auto &key) noexcept(true) { return key.likes; },
					[](const auto &key) noexcept(true) { return key.visits; })
				.Select([](const auto &tupl) noexcept(true) { return std::get<0>(tupl); })
				.Sum();
		});
	}
};
template <typename T>
struct Test<T, which::GroupBy>
{
	auto operator()() const
	{
		Context<T> context;
		auto &data = context.get();
		return test("Naive->GroupBy", [&]() {
			std::unordered_map<int, std::unordered_map<int, std::vector<T>>> groups;
			int result = 0;
			for (const auto &it : data)
				groups[it.group][it.category].push_back(it);
			for (const auto &it : groups)
				result += it.first;
			return result;
		})
		==
		test("IEnum->GroupBy", [&]() {
			return linq::make_enumerable(data)
				.GroupBy(
					[](const auto &key) noexcept(true) { return key.group; },
					[](const auto &key) noexcept(true) { return key.category; })
				.Select([](auto const &pair) { return pair.first; })
				.Sum();
		});
	}
};
template <typename T>
struct Test<T, which::OrderBy>
{
	auto operator()() const
	{
		Context<T> context;
		auto &data = context.get();

		return test("Naive->OrderBy", [&]() noexcept(true) {
			std::sort(data.begin(), data.end(), [](T const &l, T const &r)
			{
				return l.group < r.group || (l.group == r.group && //asc 
					((l.category > r.category || l.category == r.category) && //desc
					((l.likes < r.likes || l.likes == r.likes) && //asc
						l.visits > r.visits))); //desc
			});
			return 0;
		})
		==
		test("IEnum->OrderBy", [&]() {
			linq::make_enumerable(data)
				.OrderBy(
					linq::asc([](const auto &key) noexcept(true) { return key.group; }),
					linq::desc([](const auto &key) noexcept(true) { return key.category; }),
					linq::asc([](const auto &key) noexcept(true) { return key.likes; }),
					linq::desc([](const auto &key) noexcept(true) { return key.visits; })
				);
			return 0;
		});
	}
};
template <typename T>
struct Test<T, which::Custom>
{
	auto operator()() const
	{
		Context<T> context;
		auto &data = context.get();
		return test("IEnum->Custom", [&]() {
			auto enu = linq::make_enumerable(data)
				.SelectMany([](auto const &usr) { return usr.id; },
					[](auto const &usr) { return usr.group; })
				.Where([](auto const &tuple) { return std::get<0>(tuple) <= 100000; })
				.OrderBy(linq::desc([](auto const &tuple) { return std::get<1>(tuple); }))
				.Select([](auto const &tuple) { return std::get<0>(tuple); });
			int sum = 0;
			if (enu.Contains(10000))
				enu.TakeWhile([](auto const &val) { return val <= 100000; }).Each([&sum](auto const &) {  ++sum; });
			return enu.min() + enu.max() 
				   + enu.First() - enu.FirstOrDefault() +
				   + enu.Last() - enu.LastOrDefault() + sum;			
				
		});
		return 0;
	}
};

void executeTests()
{
	std::cout << "# Overhead Int" << std::endl;
	assertEquals(Test<int, which::From>()(), true);
	assertEquals(Test<int, which::Take>()(), true);
	assertEquals(Test<int, which::Skip>()(), true);
	assertEquals(Test<int, which::Where>()(), true);

	std::cout << "# Overhead User" << std::endl;
	assertEquals(Test<User, which::Select>()(), true);
	assertEquals(Test<User, which::Take>()(), true);
	assertEquals(Test<User, which::Skip>()(), true);
	assertEquals(Test<User, which::Where>()(), true);
	assertEquals(Test<User, which::SelectMany>()(), true);
	assertEquals(Test<User, which::GroupBy>()(), true);
	assertEquals(Test<User, which::OrderBy>()(), true);
	assertEquals(Test<User, which::Custom>()(), 200001);

	std::cout << "# Overhead Random User" << std::endl;
	assertEquals(Test<UserRandom, which::Select>()(), true);
	assertEquals(Test<UserRandom, which::Take>()(), true);
	assertEquals(Test<UserRandom, which::Skip>()(), true);
	assertEquals(Test<UserRandom, which::Where>()(), true);
	assertEquals(Test<UserRandom, which::SelectMany>()(), true);
	assertEquals(Test<UserRandom, which::GroupBy>()(), true);
	assertEquals(Test<UserRandom, which::OrderBy>()(), true);
	assertEquals(Test<UserRandom, which::Custom>()(), 200001);
}

int main(int, char *[])
{
	std::srand(time(0));
	std::cout << "# Overhead Tests" << std::endl;
	executeTests();
	system("pause");
	return EXIT_SUCCESS;
}
