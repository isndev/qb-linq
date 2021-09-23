#include <type_traits>
#include <functional>
#include <utility>
#include <memory>
#include <tuple>

#include <algorithm>
#include <unordered_map>
#include <vector>
#include <map>

#ifndef LINQ_H_
# define LINQ_H_
# include "linq/Utility.h"

namespace linq
{
    template<typename Iterator>
    class TState;
}

# include "linq/All.h"
# include "linq/Select.h"
# include "linq/Where.h"
# include "linq/Take.h"
# include "linq/From.h"
# include "linq/TState.h"
# include "linq/Enumerable.h"

namespace linq
{
    template<typename T>
    auto make_enumerable(T const& container) {
        return Enumerable<From<typename T::const_iterator>>(From<typename T::const_iterator>(std::cbegin(container), std::cend(container)));
    }

    template<typename T>
    auto make_enumerable(T& container) {
        return Enumerable<From<typename T::iterator>>(From<typename T::iterator>(std::begin(container), std::end(container)));
    }

    template<typename T>
    auto make_enumerable(T const& begin, T const& end) {
        return Enumerable<From<T>>(From<T>(begin, end));
    }

    template<typename T>
    auto from(T const& container) {
        return linq::From<typename T::const_iterator>(std::cbegin(container), std::cend(container));
    }

    template<typename T>
    auto from(T& container) {
        return linq::From<typename T::iterator>(std::begin(container), std::end(container));
    }

    template<typename T>
    auto range(T const& begin, T const& end) {
        return linq::From<T>(begin, end);
    }
} // namespace linq

#endif // !LINQ_H_
