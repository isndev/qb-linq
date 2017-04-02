#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <chrono>
#include <queue>

template <typename T1, typename T2>
void assertEquals(T1 t1, T2 t2)
{
	if (!(t1 == t2))
	{
		std::stringstream ss;
		ss << "Assertion failed: ";
		ss << "Expected:" << t1;
		ss << ", Got:" << t2;
		throw std::runtime_error(ss.str());
	}
}

template <typename T, typename F>
auto time(F f)
{
	auto start = std::chrono::steady_clock::now();

	auto result = f();

	auto end = std::chrono::steady_clock::now();
	auto diff = end - start;
	return std::make_pair(std::chrono::duration<double, T>(diff).count(), result);
}

template <typename F>
auto test(const std::string& name, F f, std::ostream& os = std::cout)
{
	os << "Running test '" << name << "' \t\t\t";
	os.flush();

	//try
	//{
	auto result = time<std::micro>(f);
	long long duration = result.first;

	os << "[" << duration << " us] ";
	os << "-> Success" << std::endl;
	return result.second;
	//}
	//catch (const std::exception& e)
	//{
	//	os << "-> Failed !" << std::endl;
	//	os << "\t => " << e.what() << std::endl;
	//	return decltype(f()){};
	//}
}
