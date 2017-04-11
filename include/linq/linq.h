#include <type_traits>
#include <utility>
#include <algorithm>
#include <functional>
#include <tuple>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>

#ifndef LINQ_H_
# define LINQ_H_
# include "linq/Utility.h"
# include "linq/Iterator.h"

namespace linq
{
	template<typename Proxy, typename Base_It, typename Out, iterator_type ItType>
	class IState;
}

# include "linq/All.h"
# include "linq/GroupBy.h"
# include "linq/OrderBy.h"
# include "linq/SelectWhere.h"
# include "linq/Select.h"
# include "linq/Where.h"
# include "linq/Take.h"
# include "linq/From.h"
# include "linq/IState.h"
# include "linq/IEnumerable.h"

namespace linq
{
	template<typename T>
	auto make_enumerable(T const &container)
	{
		return std::move(IEnumerable<From<typename T::const_iterator>>(From<typename T::const_iterator>(std::begin(container), std::end(container))));
	}
	template<typename T>
	auto make_enumerable(T &container)
	{
		return std::move(IEnumerable<From<typename T::iterator>>(From<typename T::iterator>(std::begin(container), std::end(container))));
	}
	template<typename T>
	auto make_enumerable(T const &begin, T const &end)
	{
		return std::move(IEnumerable<From<T>>(From<T>(begin, end)));
	}

	template<typename T>
	auto from(T const &container)
	{
		return std::move(linq::From<typename T::const_iterator>(std::begin(container), std::end(container)));
	}
	template<typename T>
	auto from(T &container)
	{
		return std::move(linq::From<typename T::iterator>(std::begin(container), std::end(container)));
	}
	template<typename T>
	auto range(T const &begin, T const &end)
	{
		return std::move(linq::From<T>(begin, end));
	}
}

#endif // !LINQ_H_
