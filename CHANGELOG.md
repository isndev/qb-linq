# Changelog

All notable changes to **qb-linq** are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html). The canonical version is **`project(qb-linq VERSION …)`** in `CMakeLists.txt` — see [`docs/VERSIONING.md`](docs/VERSIONING.md).

## [Unreleased]

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

[1.2.1]: https://github.com/isndev/qb-linq/compare/v1.2.0...v1.2.1
[1.2.0]: https://github.com/isndev/qb-linq/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/isndev/qb-linq/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/isndev/qb-linq/releases/tag/v1.0.0
