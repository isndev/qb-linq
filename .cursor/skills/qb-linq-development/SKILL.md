---
name: qb-linq-development
description: >-
  Develop or debug the qb-linq header-only C++17 LINQ library (qb::linq): locate declarations
  and definitions, extend iterators and views, run qb_linq_tests and optional benchmarks.
  Use when editing include/qb/linq, adding terminals or lazy views, or writing tests for this repo.
---

# qb-linq development

## When this applies

Use this skill for work **inside the qb-linq repository**: changing pipelines, iterators, materializers, tests, or benchmarks—not for application code that only consumes `qb::linq::from(...)`.

## Required context (read first)

1. **`docs/BUILDING.md`** — CMake target **`qb::linq`**, options, presets, **`qb_linq_tests`** / **`qb_linq_benchmark`** / **`qb_linq_docs`**.
2. **`docs/LLM_CONTEXT.md`** — include graph, handle types, out-of-line split (`query.h` vs `extra_views.h`), `materialized_range` pitfall.
3. **`AGENTS.md`** — checklist and file map.
4. **`include/qb/linq.h`** — Doxygen **`linq`** group: public caveats.
5. **`CONTRIBUTING.md`** — tests, PR checklist, **CHANGELOG** for user-visible changes.

## Workflow

1. **Locate the API** — Methods on `enumerable` trace to `query_range_algorithms` in `query_range.h`.
2. **Declaration vs definition** — Declarations in `query_range.h`; definitions in `query.h` or `extra_views.h` per `LLM_CONTEXT.md`.
3. **New iterators** — Extend patterns in `iterators.h` or `extra_views.h`; respect iterator category rules (`clamp_iterator_category_t` where appropriate).
4. **Verify** — Build **`qb_linq_tests`**, run **`ctest`**. For perf-sensitive code, enable **`QB_BUILD_BENCHMARKS`** and keep **`qb_linq_benchmark`** building.
5. **Portability** — Validate on multiple compilers when possible (GCC, Clang, MSVC).

## Pitfalls

- Do not break **`materialized_range`** safe construction documented in **`query.h`**.
- **`select_many`** yields a **tuple** of projections per element, not a flattened sequence (not C# `SelectMany`).
- **`scan_iterator`** must not outlive **`scan_view`** (`F*` into the view).
- **`reverse()`** + **`take` / `take_while`:** use the dedicated **`reversed_view`** specializations in **`query.h`**; new logical-end iterators must not break **`reverse_iterator`** equality or bounds.

## Output expectations

- **Minimal diffs**, consistent style and Doxygen.
- **Tests** for behaviour changes.
- **English** commit messages; state what and why.
