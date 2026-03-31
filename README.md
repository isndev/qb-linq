<div align="center">

# qb-linq

### *Lazy LINQ-style pipelines for C++17 — header-only, zero dependencies beyond the STL*

[![CI](https://github.com/isndev/qb-linq/actions/workflows/cmake.yml/badge.svg)](https://github.com/isndev/qb-linq/actions/workflows/cmake.yml)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B&logoColor=white)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.22%2B-064F8C?logo=cmake&logoColor=white)](https://cmake.org/cmake/help/latest/release/3.22.html)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Header-only](https://img.shields.io/badge/library-header--only-blue.svg)](#quick-start-cmake)
[![Platforms](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)](#requirements)

**`#include <qb/linq.h>`** · namespace **`qb::linq`** · target **`qb::linq`**

[Features](#why-qb-linq) · [Quick start](#quick-start-cmake) · [**Full API**](#complete-api-reference) · [Examples](#examples) · [Build & test](#build-options-this-repository)

</div>

---

**qb-linq** brings **Enumerable-style** method chains to modern C++: **`where`**, **`select`**, **`order_by`**, **`group_by`**, **`join`**, set algebra, **percentiles / variance**, **`zip`** / **`zip_fold`**, **`scan`**, **`chunk`**, materializers, and **fused** terminals — without `std::ranges`, without a runtime library. Views are **lazy**; work happens when you iterate or call a **terminal**.

---

## Why qb-linq?

| | |
|---:|---|
| **LINQ surface** | Filters, projections, sorting, grouping, joins, sets, stats, factories — see [**complete API**](#complete-api-reference) below. |
| **Lazy by default** | Pipelines are cheap to build; execution is driven by iterators and terminals. |
| **`where` is smart** | O(1) construction; first `begin()` runs a single `find_if` and **caches** the result (CRTP `query_range_algorithms`). |
| **Fused hot paths** | `sum_if`, `count_if`, `any_if`, `all_if`, `none_if`, `aggregate` / `fold` — **one pass**, no extra `where_iterator` layer. |
| **CMake-native** | `find_package(qb-linq)` or `add_subdirectory` → link **`qb::linq`**. |

---

## Requirements

- **C++17** — GCC, Clang, MSVC  
- **CMake 3.22+** for the bundled config / presets  
- **Linux · macOS · Windows** (verified in CI)

---

## Quick start (CMake)

### Installed package

```cmake
find_package(qb-linq 1.0 CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE qb::linq)
target_compile_features(your_target PRIVATE cxx_std_17)
```

### `add_subdirectory`

```cmake
add_subdirectory(third_party/qb-linq)
target_link_libraries(your_target PRIVATE qb::linq)
```

### Hello pipeline

```cpp
#include <iostream>
#include <vector>
#include <qb/linq.h>

int main() {
    std::vector<int> v{1, 2, 3, 4, 5, 6};
    int s = qb::linq::from(v)
                .where([](int x) { return x % 2 == 0; })
                .select([](int x) { return x * x; })
                .sum();
    std::cout << s << '\n';   // 4 + 16 + 36 = 56
}
```

Include **only** [`<qb/linq.h>`](include/qb/linq.h); other headers stay under `qb/linq/`.

---

## Design at a glance

1. **`enumerable<Handle>`** — fluent façade; each lazy step returns a new `enumerable`.
2. **`query_range_algorithms` (CRTP)** — algorithms use `derived().begin()` / `end()`.
3. **Terminals** — `sum`, `to_vector`, `first`, `order_by`, `group_by`, … trigger real work (many materialize via `shared_ptr`).
4. **Caveats** — `select_many` = **tuple per row** (not C# flatten); `except`/`intersect` need **hashable** `value_type`; `zip`/`concat` inner pipelines may need **copy-assignable** functors (`+[](){}`). Full notes in [`qb/linq.h`](include/qb/linq.h).

---

## Complete API reference

All operations below live in **`namespace qb::linq`**. Method names use **`snake_case`**.  
Unless noted, methods are **`const`** and return a new **`enumerable`** for lazy ops, or a **value / materialized range** for terminals.

### Free functions — factories & entry points

| API | Role |
|-----|------|
| **`from(container&)`** / **`from(container const&)`** | Start from any iterable (`begin`/`end`). |
| **`from(Iter b, Iter e)`** | Iterator pair. |
| **`range(b, e)`** | Alias of `from(b, e)`. |
| **`as_enumerable(...)`** | Overloads: `enumerable&` / `const&` / `&&` (identity); container ref → `from`; `(Iter b, Iter e)` → `from`. |
| **`iota(T first, T last)`** | Lazy half-open `[first, last)` (no allocation). |
| **`empty<T>()`** | Empty sequence. |
| **`once(T&&)`** | Single-element sequence. |
| **`repeat(T&&, std::size_t n)`** | Repeat value `n` times. |
| **`asc(key)`** / **`desc(key)`** | Build ascending / descending key projection for `order_by`. |
| **`make_filter<CustomBase>(key, args...)`** | Custom comparator base for `order_by` (e.g. `make_filter<MyLess>([](int x){ return x; })`). |
| **CTAD** | **`enumerable(h)`** deduces `enumerable<std::decay_t<decltype(h)>>`. |

### `enumerable` — iteration

| API | Role |
|-----|------|
| **`begin()`** / **`end()`** | Range-based `for`, STL iterator usage. |
| **`value_type`** | Element / projected type. |

### Lazy transformation & slicing

| API | Role |
|-----|------|
| **`select(f)`** | Map each element. |
| **`select_many(fs...)`** | **Tuple** of one projection per loader per row (not nested flattening). |
| **`where(pred)`** | Filter; first `begin()` may cache first match. |
| **`of_type<U>()`** | Polymorphic **`dynamic_cast`** filter (`value_type` must be pointer, pointee polymorphic). |
| **`skip(n)`** | Drop first `n`. |
| **`skip_while(pred)`** | Drop while predicate true. |
| **`take(n)`** | Take `n` (`int`; negative magnitude like LINQ; `INT_MIN` → empty). |
| **`take_while(pred)`** | Take while predicate true. |
| **`skip_last(n)`** / **`take_last(n)`** | Materialize then trim head/tail. |
| **`reverse()`** | Reverse view (bidirectional base). |

### Composition & extra lazy views

| API | Role |
|-----|------|
| **`concat(rhs)`** | Concatenate another range (same `value_type`). |
| **`zip(rhs)`** | Pairwise zip; stops at shorter length. |
| **`zip_fold(rhs, init, f)`** | Single-pass fold over pairs `f(acc, left, right)`; same length rule as `zip`. |
| **`append(v)`** / **`prepend(v)`** | Single element after/before sequence. |
| **`default_if_empty()`** / **`default_if_empty(def)`** | Sentinel when empty. |
| **`enumerate()`** | `(index, element)` pairs. |
| **`scan(seed, f)`** | Running fold; yields each accumulator. |
| **`chunk(size)`** | Chunks as `std::vector` slices (lazy per chunk). |
| **`stride(step)`** | Every `step`-th element (`0` clamped to `1`). |
| **`distinct()`** | First occurrence by value (hash). |
| **`distinct_by(keyfn)`** | First occurrence per key. |

### Ordering & grouping

| API | Role |
|-----|------|
| **`order_by(key_filters...)`** | **Materialize** + stable multi-key sort (`asc` / `desc` / `make_filter`). |
| **`group_by(keys...)`** | **Materialize** nested map / vectors (multi-level keys). |
| **`to_lookup(key)`** | Alias of **`group_by(key)`**. |
| **`asc()`** / **`desc()`** | On **materialized** sorted views: identity / reverse iterator view (when supported by handle). |

### Relational

| API | Role |
|-----|------|
| **`join(inner, outer_key, inner_key, result_sel)`** | Inner join; flat result rows. |
| **`group_join(inner, outer_key, inner_key)`** | Outer + inner groups per key. |

### Set-style (after materialization / hash)

| API | Role |
|-----|------|
| **`except(rhs)`** | Left minus right (unordered set of RHS values). |
| **`intersect(rhs)`** | Filter left by membership in RHS. |
| **`union_with(rhs)`** | `concat` + `distinct`. |

### Materialization — collections & maps

| API | Role |
|-----|------|
| **`to_vector()`** | Copy to `std::vector` (random-access reserve when possible). |
| **`materialize()`** | Alias of **`to_vector()`**. |
| **`to_set()`** / **`to_unordered_set()`** | Unique sets. |
| **`to_map(keyf, elemf)`** | `std::map` (ordered keys). |
| **`to_unordered_map(keyf, elemf)`** | `std::unordered_map` (last wins on duplicate key). |
| **`to_dictionary(keyf, elemf)`** | `unordered_map`; **throws** on duplicate key. |

### Search & indexing

| API | Role |
|-----|------|
| **`contains(v)`** | Linear search `==`. |
| **`contains(v, eq)`** | Custom equality. |
| **`index_of(v)`** / **`last_index_of(v)`** | `std::optional<std::size_t>`. |
| **`index_of(v, eq)`** / **`last_index_of(v, eq)`** | With custom equality. |

### Counting & boolean quantifiers

| API | Role |
|-----|------|
| **`any()`** | Non-empty? |
| **`count()`** / **`long_count()`** | `std::distance` (full scan). |
| **`count_if` / `long_count_if`** | Predicate count (fused). |
| **`any_if` / `all_if` / `none_if`** | Fused predicate quantifiers. |

### Element access & single-element contracts

| API | Role |
|-----|------|
| **`first()`** / **`last()`** | Throws if empty. |
| **`first_or_default()`** / **`last_or_default()`** | Value or `value_type{}`. |
| **`first_if` / `first_or_default_if`** | Predicate. |
| **`last_if` / `last_or_default_if`** | Predicate (last match). |
| **`element_at(i)`** / **`element_at_or_default(i)`** | Zero-based index. |
| **`single()`** / **`single_or_default()`** | Exactly one element (or default). |
| **`single_if` / `single_or_default_if`** | Exactly one match for predicate. |
| **`sequence_equal(rhs)`** / **`sequence_equal(rhs, comp)`** | Pairwise compare two ranges. |

### Min / max (by value or key)

| API | Role |
|-----|------|
| **`min()`** / **`max()`** / **`min_max()`** | Throws if empty. |
| **`min(comp)`** / **`max(comp)`** / **`min_max(comp)`** | Custom strict weak ordering. |
| **`min_or_default()`** / **`max_or_default()`** | Empty → default. |
| **`min_by` / `max_by`** | Compare by projected key. |
| **`min_by_or_default` / `max_by_or_default`** | Empty → default. |

### Aggregates & folds

| API | Role |
|-----|------|
| **`sum()`** | `+=` from `value_type{}`. |
| **`sum_if(pred)`** | Fused filtered sum. |
| **`average()`** / **`average_if(pred)`** | Mean (`double`); throws if empty / no match. |
| **`aggregate(init, f)`** / **`fold`** | Left fold with seed (`fold` = alias). |
| **`reduce(f)`** | Fold without seed; first element is initial acc; empty throws. |

### Order statistics & spread

| API | Role |
|-----|------|
| **`percentile(p)`** / **`percentile_by(p, key)`** | `p` in [0,100]; sort-based interpolation. |
| **`median()`** / **`median_by(key)`** | Percentile 50. |
| **`variance_population()`** / **`variance_sample()`** | Welford; arithmetic `value_type` or use `*_by`. |
| **`variance_population_by` / `variance_sample_by`** | Project before stats. |
| **`std_dev_population` (+ `_by`)** / **`std_dev_sample` (+ `_by`)** | `sqrt` of variance. |

### Side effects

| API | Role |
|-----|------|
| **`each(f)`** | Apply `f` to each element; returns **copy** of current stage for safe chaining (`const&` / `&` overloads). |

### Map / lookup bracket (when handle supports it)

| API | Role |
|-----|------|
| **`operator[](key)`** | Available on **map-backed** materialized ranges (e.g. `to_dictionary`, grouped structures) when the underlying container exposes `at`. |

> **Coverage:** the table above lists **every** forwarding entry point on **`enumerable`** plus **all** documented free factories in [`enumerable.h`](include/qb/linq/enumerable.h). Internal iterator adaptors (`select_iterator`, …) live in [`iterators.h`](include/qb/linq/iterators.h); combinators (`concat_view`, `zip_view`, …) in [`detail/extra_views.h`](include/qb/linq/detail/extra_views.h).

---

## Examples

### Pipeline: filter → map → take → materialize

```cpp
std::vector<int> data{3, 1, 4, 1, 5, 9, 2, 6};
auto v = qb::linq::from(data)
             .where([](int x) { return x > 2; })
             .select([](int x) { return x * x; })
             .take(3)
             .to_vector();
```

### Fused aggregate

```cpp
std::vector<int> xs{1, 2, 3, 4, 5, 6};
auto n = qb::linq::from(xs).count_if([](int x) { return x % 2 == 0; });
auto s = qb::linq::from(xs).sum_if([](int x) { return x % 2 == 0; });
```

### Multi-key sort

```cpp
struct Row { int dept; int id; };
std::vector<Row> rows{{2, 10}, {1, 5}, {1, 3}, {2, 7}};
auto sorted = qb::linq::from(rows)
                  .order_by(qb::linq::asc([](Row const& r) { return r.dept; }),
                            qb::linq::desc([](Row const& r) { return r.id; }))
                  .to_vector();
```

### `join` / `group_join`

```cpp
struct Person { int id; std::string name; };
struct Score { int person_id; int points; };
std::vector<Person> people{{1, "Ann"}, {2, "Bob"}};
std::vector<Score> scores{{1, 10}, {1, 11}, {2, 20}};

auto joined = qb::linq::from(people).join(scores,
    [](Person const& p) { return p.id; },
    [](Score const& s) { return s.person_id; },
    [](Person const& p, Score const& s) {
        return p.name + ":" + std::to_string(s.points);
    });

auto grouped = qb::linq::from(people).group_join(scores,
    [](Person const& p) { return p.id; },
    [](Score const& s) { return s.person_id; });
```

### Sets & stats

```cpp
std::vector<int> a{1, 2, 3, 4}, b{3, 4, 5};
auto u = qb::linq::from(a).union_with(b).to_vector();
std::vector<double> xs{1, 2, 3, 4, 5};
double m = qb::linq::from(xs).median();
double p = qb::linq::from(xs).percentile(75.0);
```

### `zip` · `scan` · `enumerate` · factories

```cpp
std::vector<int> a{1, 2, 3}, b{10, 20, 30};
for (auto const& pr : qb::linq::from(a).zip(b)) { /* pr.first, pr.second */ }

for (int x : qb::linq::from(a).scan(0, [](int acc, int v) { return acc + v; })) { }

for (auto const& pr : qb::linq::from(std::string("abc")).enumerate()) { }

auto xs = qb::linq::iota(0, 10);
auto one = qb::linq::once(42);
auto pat = qb::linq::repeat(std::string("x"), 3);
auto none = qb::linq::empty<int>();
```

### `each` (in-place)

```cpp
std::vector<int> data{1, 2, 3};
qb::linq::from(data).each([](int& x) { x *= 2; });
```

---

## C# LINQ ↔ qb-linq (quick map)

| C# | qb-linq |
|----|---------|
| `Where` | `where` |
| `Select` | `select` |
| `SelectMany` (flatten) | use `select` + `concat` / loops; **`select_many`** = tuple bundle |
| `OrderBy` / `ThenBy` | `order_by(asc(...), desc(...))` |
| `GroupBy` | `group_by` |
| `Join` / `GroupJoin` | `join` / `group_join` |
| `ToList` | `to_vector` / `materialize` |
| `ToDictionary` | `to_dictionary` / `to_map` / `to_unordered_map` |
| `Distinct` | `distinct` / `distinct_by` |
| `Union` / `Except` / `Intersect` | `union_with` / `except` / `intersect` |
| `Zip` | `zip` |
| `Aggregate` | `aggregate` / `reduce` / `fold` |
| `Sum` / `Average` / `Min` / `Max` | `sum`, `average`, `min`, `max`, `min_max`, … |
| `Any` / `All` / `Count` | `any`, `all_if`, `count`, `count_if`, … |
| `Range` / `Repeat` / `Empty` | `iota`, `repeat`, `empty` |

---

## Build options (this repository)

| CMake option | Default | Purpose |
|--------------|---------|---------|
| `QB_BUILD_TESTS` | **ON** if top-level | GoogleTest → `ctest`, `qb_linq_tests` |
| `QB_BUILD_BENCHMARKS` | OFF | Google Benchmark → `qb_linq_benchmark` |
| `QB_LINQ_BUILD_DOCS` | OFF | Doxygen → `qb_linq_docs` |

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DQB_BUILD_TESTS=ON -DQB_BUILD_BENCHMARKS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/benchmarks/qb_linq_benchmark
```

**Presets:** `cmake --preset dev` · `cmake --build build/dev` · `ctest --preset dev`  
(`release`, `ci`, `docs` — see [`CMakePresets.json`](CMakePresets.json).)

---

## Repository layout

```
include/qb/linq.h
include/qb/linq/
  enumerable.h
  query.h
  iterators.h
  detail/
    query_range.h
    extra_views.h
    group_by.h
    order.h
    type_traits.h
tests/                 # 210+ GoogleTest cases
benchmarks/            # Google Benchmark suite
cmake/
```

---

## Documentation

- **Module docs + caveats:** [`qb/linq.h`](include/qb/linq.h) — Doxygen groups **`qb`** (framework) and **`linq`** (`qb/linq`; see `ingroup` tags in the headers).
- **HTML:** `QB_LINQ_BUILD_DOCS=ON` → target **`qb_linq_docs`**.

---

## License

[MIT License](LICENSE) — Copyright (c) 2026 ISNDEV

---

## Contributing

PRs welcome. Behaviour changes should come with tests in `tests/` and should keep `QB_BUILD_BENCHMARKS=ON` builds green.
