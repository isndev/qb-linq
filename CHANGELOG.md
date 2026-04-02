# Changelog

All notable changes to **qb-linq** are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html). The canonical version is **`project(qb-linq VERSION …)`** in `CMakeLists.txt` — see [`docs/VERSIONING.md`](docs/VERSIONING.md).

## [Unreleased]

### Fixed

- **`last()`:** Now works with **forward-only** iterators (linear scan fallback). Previously required bidirectional iterators (`std::prev`). Bidirectional+ paths still use `std::prev` and return by reference; forward-only returns by value.
- **`last_if`:** No longer requires **default-constructible** `value_type`. Uses `std::optional` internally instead of pre-initializing `value_type{}`.
- **`single_if`:** No longer requires **default-constructible** `value_type`. Uses `std::optional` internally instead of pre-initializing `value_type{}`.

### Added

- **`flat_map(f)` / `select_many_flatten(f)`:** True C# `SelectMany` — lazy one-to-many projection + flatten. `f(*it)` returns a sub-range; all inner elements are yielded sequentially. Empty inner ranges are skipped automatically.
- **`sliding_window(n)`:** Overlapping sliding windows of `n` elements, each yielded as a `std::vector` by value. Slides by one element per step. Complements `chunk(n)` (non-overlapping batches). Useful for moving averages, signal processing, n-gram analysis.
- **`cast<T>()`:** Lazy `static_cast<T>` of each element. Complement to `of_type<T>()` (which uses `dynamic_cast` + filter). Implemented via `select`.
- **`aggregate_by(keyf, seed, reducer)`:** Group by key and reduce each group in a single pass — no intermediate collections. Returns `materialized_range` over `vector<pair<Key, Acc>>` in first-seen key order. Equivalent to `group_by(key)` + per-group `aggregate`, but O(1) memory per group.
- **`reduce_by(keyf, reducer)`:** Like `aggregate_by` but uses the first element per key as the initial accumulator (no seed needed). Returns `materialized_range` over `vector<pair<Key, value_type>>`.
- **Tests (`linq_audit_test.cpp`):** 81 audit tests covering forward-only `last()`, non-default-constructible types with `last_if`/`single_if`, append/prepend edge cases, enumerate, chunk, stride, repeated enumeration, laziness verification, empty/single-element inputs, join edge cases, duplicate handling, factories, sequence equality, scan, zip, default-if-empty, aggregates, exceptions, zip-fold, and `std::forward_list` (strictly forward iterators).
- **Tests (`linq_new_features_test.cpp`):** 40 tests for `flat_map`, `sliding_window`, `cast<T>()`, `aggregate_by`, `reduce_by` covering empty ranges, single elements, chaining, laziness, edge cases, and real-world patterns (moving average, string concatenation by key, max-by-key).

## [1.3.0] - 2026-04-01

### Fixed

- **`order_by`:** **`std::stable_sort`** for stable / **.NET `OrderBy`**-style ordering when keys compare equal (replaces **`std::sort`**, which could reorder ties). Sort predicate uses **`value_type const&`** so **MSVC** accepts calls from **`stable_sort`** internals. Regression: **`OrderAdvanced.StableForDuplicateSortKey`**.
- **`except` / `intersect`:** match **.NET `Enumerable.Except` / `Enumerable.Intersect`** — each distinct **`value_type`** at most once, **first-seen** order of the left sequence. Documented in **`qb/linq.h`**, **`query_range.h`**, and README. **Migration:** code that depended on multiset-style output should use an explicit pass or keyed **`where`** instead.

### Added

- **LINQ / .NET-style APIs (lazy or materializing as documented):** **`select_indexed`**, **`where_indexed`**, **`zip(r2, r3)`** (three-way, **`std::tuple`** of references), **`except_by`**, **`intersect_by`**, **`union_by`**, **`count_by`**, **`try_get_non_enumerated_count()`**, **`left_join`**, **`right_join`** (outer joins via **`std::optional`** for the missing side). See README **Complete API** and **`qb/linq.h`** caveats.
- **Docs:** **`qb/linq.h`**, README, **`docs/LLM_CONTEXT.md`** — API tables, C# quick map, **`from`** / lazy pipeline lifetime, set-op and outer-join notes.
- **Tests:** **`linq_linq_parity_extended_test.cpp`**, **`linq_robustness_parity_test.cpp`**, **`linq_join_set_test.cpp`**; extended **`linq_property_fuzz_test.cpp`** (inner-join row count vs nested loop, outer joins, **`zip`×3**, **`count_by`**, key-based set ops) and **`linq_surfaces_test.cpp`** (compile-time / surface checks for new APIs).
- **CI / CMake:** Ubuntu **`sanitize`** job (**ASan** + **UBSan** on **`qb_linq_tests`**); **`sanitize`** configure preset and **`ctest --preset sanitize`** ([**`docs/BUILDING.md`**](docs/BUILDING.md)).

## [1.2.1] - 2026-04-03

### Added

- **Tests (`linq_predicates_robustness_test.cpp`):** `concat` with const RHS range; `zip` with mutable LHS / const RHS; `take` / `skip` + `where` predicate call-count regressions; **`reverse()` after `take` / `take_while`** — full prefix, empty, single element, exhaustion at physical end, `take(0)`, `n` larger than length, `skip` + `take_while`.
- **Tests:** `linq_scan_and_iterator_layout_test.cpp` — `scan` copy / `enumerable` semantics, stateful fold, parity with `std::partial_sum`, EBO `sizeof` checks via a minimal one-pointer iterator (and `vector::iterator` when pointer-sized).
- **Benchmarks:** `benchmarks/bench_scan_contract.cpp` — scan walk vs per-step iterator copy; `select` with empty vs fat functor.

### Fixed

- **`qb::linq::reverse_iterator`:** random-access-only members (`+=`, `+`, `-=`, `-`, `[]`) use dependent SFINAE (`enable_if_ra_base<BaseIt>`) so the class template is valid when the base iterator is **bidirectional-only** (e.g. `take_n_iterator` after category clamp). Fixes `take(n).reverse()` instantiation.
- **`reversed_view`:** full specializations for **`take_n_iterator`** and **`take_while_iterator`** — `take(n).reverse()` and `take_while(pred).reverse()` iterate only within the logical prefix (generic `reverse_iterator(src.end())` was wrong for these sentinels). For **`take_while`**, reversal uses **`reverse_iterator<BaseIt>`** on underlying bases because **`take_while_iterator::operator==`** is asymmetric and must not be nested inside **`reverse_iterator`** for the two bounds.
- **`concat_iterator`:** `reference` is the **common reference** of both legs (`decltype(false ? *It1& : *It2&)`) so pipelines like `from(non_const_vector).union_with(const_vector)` compile (`int&` vs `const int&`).
- **`take_n_iterator::operator++`:** advance the underlying iterator only while `remaining_ < 0` after incrementing `remaining_`, so `take(n)` does not run trailing work (e.g. `where` predicates) after the n-th element.

### Changed

- **`enumerable::each`:** removed **`[[nodiscard]]`** — traversal runs before return; discarding the result is normal for side effects. Documented in Doxygen.
- **`scan` / `scan_view`:** fold functor **`F`** is stored **in the view**; iterators hold a raw **`F*`** (no `shared_ptr`). **Lifetime:** `scan_iterator` must **not** outlive the **`scan_view`** it came from.
- **`select_iterator`, `where_iterator`, `take_while_iterator`:** callables via **`detail::compressed_fn<F>`** with **EBO** for empty class functors. **MSVC:** **`__declspec(empty_bases)`** and **`compressed_fn<F,true>`** where needed.
- **Docs:** README API notes for **`reverse()`**, **`to_vector()`** return type, and **`linq.h`** caveats (`to_vector` / `materialize`, `scan` lifetime, `reverse` + `take` / `take_while`). **`docs/LLM_CONTEXT.md`** — `reversed_view` / `reverse` correctness and regression test names.

## [1.2.0] - 2026-04-02

### Added

- **`single_header/linq.h`** — amalgamated drop-in header (same API as **`<qb/linq.h>`**); scripts **`scripts/amalgamate_single_header.{py,ps1,sh}`**, CMake target **`qb_linq_single_header`**, smoke test **`qb_linq_single_header_test`**.
- **`.gitattributes`** — **`*.sh`**, amalgamation **`.py`**, and **`single_header/linq.h`** use **`eol=lf`**.

### Fixed

- **`qb_linq_single_header`** on Unix runs **`amalgamate_single_header.py`** via **`python3`/`python`** instead of invoking the **`.sh`** wrapper through **`sh`**.

## [1.1.0] - 2026-04-01

### Added

- **[`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md)** (Contributor Covenant 2.1).
- **Dependabot** for GitHub Actions (`.github/dependabot.yml`).
- Issue chooser **contact links** (security, CoC, contributing).
- **Doxygen `INPUT`** includes linked repo Markdown (and **`.cursor/.../SKILL.md`**) so README / agent docs cross-links resolve in CI.

### Changed

- **CI `docs` job** fails if Doxygen writes a non-empty **`doxygen-qb-linq.warnings.log`**.
- **Doxygen** strips both `include/` and the generated **`version.h`** include root for shorter file paths in HTML.
- **`HAVE_DOT = NO`** in **`docs/Doxyfile.in`** so HTML builds do not require Graphviz on runners.

### Fixed

- **`CMakeLists.txt`** **`QB_LINQ_DOXYFILE_IN`** points at **`docs/Doxyfile.in`** (not removed **`doc/`** path).
- **`AGENTS.md`** no longer hardcodes a library version number (use **`CMakeLists.txt`** / **`docs/VERSIONING.md`**).
- **`.gitignore`** documents **`doc_doxygen/`** and related outputs; keeps legacy **`doc/`** ignores.
- **Doxygen / CI:** custom **`header.html`** omits **`PROJECT_ICON`** / **`COPY_CLIPBOARD`** blocks (incompatible markers on Ubuntu’s Doxygen). README hero tagline uses bold instead of `###` under `#` (section nesting warning).

## [1.0.0] - 2026-03-31

### Added

- Header-only **`qb::linq`** API: lazy LINQ-style pipelines for **C++17** (`#include <qb/linq.h>`).
- CMake **INTERFACE** target **`qb::linq`**, install + **`find_package(qb-linq CONFIG)`** (`SameMajorVersion`).
- Generated **`qb/linq/version.h`** from CMake `project(VERSION)` (macros + `qb::linq::version`).
- GoogleTest suite (**`qb_linq_tests`**) and Google Benchmark suite (**`qb_linq_benchmark`**).
- Doxygen HTML docs (**`qb_linq_docs`**, Doxygen Awesome under `docs/doxygen/`).
- Guides under **`docs/`** (building, versioning, LLM context); **CONTRIBUTING**, **SECURITY**, **AGENTS**; GitHub issue/PR templates; documentation moved from **`doc/`** to **`docs/`**.

[Unreleased]: https://github.com/isndev/qb-linq/compare/v1.3.0...HEAD
[1.3.0]: https://github.com/isndev/qb-linq/compare/v1.2.1...v1.3.0
[1.2.1]: https://github.com/isndev/qb-linq/compare/v1.2.0...v1.2.1
[1.2.0]: https://github.com/isndev/qb-linq/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/isndev/qb-linq/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/isndev/qb-linq/releases/tag/v1.0.0
