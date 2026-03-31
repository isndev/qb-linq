# qb-linq — LLM / agent context (implementation reference)

**Purpose:** Dense map of the **header-only** C++17 library **`qb::linq`** (CMake project **qb-linq**, interface target **`qb::linq`**) so tools can reason about headers, boundaries, and contracts without re-reading the whole tree.

**Companion docs (repo-grounded):**

| Doc | Content |
|-----|---------|
| **[`README.md` (docs/)](README.md)** | Index of all guides in this folder |
| **[`AGENTS.md`](../AGENTS.md)** | Agent checklist, file map, verification |
| **[`BUILDING.md`](BUILDING.md)** | CMake options, presets, `qb_linq_tests` / `qb_linq_single_header_test` / `qb_linq_benchmark` / `qb_linq_docs` / `qb_linq_single_header` |
| **[`VERSIONING.md`](VERSIONING.md)** | SemVer, `version.h`, releases + changelog |
| **[`CHANGELOG.md`](../CHANGELOG.md)** | Release history |
| **[`CONTRIBUTING.md`](../CONTRIBUTING.md)** | PR workflow for humans |
| **[`README.md`](../README.md)** | User API, examples, layout summary |
| **[`SECURITY.md`](../SECURITY.md)** | Vulnerability reporting |
| **[`CODE_OF_CONDUCT.md`](../CODE_OF_CONDUCT.md)** | Community standards |

**Cursor skill:** [`.cursor/skills/qb-linq-development/SKILL.md`](../.cursor/skills/qb-linq-development/SKILL.md).

**Stable public include:** `<qb/linq.h>` → `include/qb/linq.h` → `qb/linq/version.h` (CMake-generated under `generated_include/`) → `enumerable.h` (transitive). Optional amalgam: **`single_header/linq.h`** + `-I single_header` + `#include <linq.h>` (regenerate **`qb_linq_single_header`**). **`detail/*`** names are not a stable public API unless exposed through `enumerable` behavior. **Version:** `project(VERSION)` in root `CMakeLists.txt` → **`docs/VERSIONING.md`**.

**Namespace:** `qb::linq` (`qb::linq::detail` for helpers).

---

## 1. Architectural invariants

| Invariant | Detail |
|-----------|--------|
| **CRTP algorithms** | `detail::query_range_algorithms<Derived, Iter>` in `query_range.h`. `Derived const& derived()` → `static_cast<Derived const&>(*this)`. Algorithms call `derived().begin()` / `derived().end()`. |
| **Iterator type parameter** | `Iter` is explicit on `query_range_algorithms` to break incomplete-type cycles between views and algorithm mixin. |
| **Lazy vs eager** | Lazy: building a view is cheap; work on iterator advance / dereference or when a **terminal** runs. Eager: terminals that return scalars, `each`, or materializers that allocate and fill containers. |
| **Ownership** | Most materialized results are `materialized_range<IteratorIntoOwner, Owner>` with `std::shared_ptr<Owner>`. Iterators remain valid while the `materialized_range` (or copies sharing the `shared_ptr`) live. |
| **No `std::ranges`** | C++17 only; all composition is custom iterators + CRTP. |

---

## 2. Include dependency graph (logical)

```
qb/linq.h
  └─ qb/linq/enumerable.h
        └─ qb/linq/query.h
              ├─ qb/linq/detail/query_range.h
              │     ├─ qb/linq/detail/group_by.h
              │     ├─ qb/linq/detail/order.h
              │     └─ qb/linq/detail/type_traits.h
              ├─ qb/linq/iterators.h
              │     └─ qb/linq/detail/type_traits.h
              └─ qb/linq/detail/extra_views.h
                    (defines concat/zip/scan/chunk/stride/distinct/enumerate/default_if_empty/empty/single/repeat
                     + out-of-line bodies for several `query_range_algorithms` methods)
```

**Rule:** `extra_views.h` is included **after** `query_range.h` declarations so return types like `concat_view<…>` are complete where definitions need them.

---

## 3. Core types (handles)

| Type | File | Role |
|------|------|------|
| `query_state<Iter>` | `query.h` | Stores `begin_`, `end_`; exposes `begin()`/`end()`. Base for many views. Inherits `query_range_algorithms<query_state<Iter>, Iter>`. |
| `from_range<RawIter>` | `query.h` | `query_state<basic_iterator<RawIter>>` — wraps raw iterators in class-type `basic_iterator`. |
| `select_view<BaseIt,F>` | `query.h` | `query_state<select_iterator<BaseIt,F>>`. |
| `where_view<BaseIt,P>` | `query.h` | **Special:** inherits `query_range_algorithms<where_view<…>, where_iterator<…>>` directly (not `query_state`). Caches first match in `mutable std::optional<BaseIt> first_` on first `begin()`. |
| `take_n_view<BaseIt>` | `query.h` | `take_n_iterator` with signed “remaining” budget; `take(n)` normalizes negative/`INT_MIN`. |
| `take_while_view<BaseIt,P>` | `query.h` | Logical end via `take_while_iterator::operator==`. |
| `subrange<Iter>` | `query.h` | `skip` / `skip_while` return type. |
| `reversed_view<Iter>` | `query.h` | `std::reverse_iterator` over source range. |
| `iota_view<T>` | `query.h` | `query_range_algorithms<iota_view<T>, iota_iterator<T>>`; half-open `[first,last)` by value. |
| `materialized_range<Iter,Owner>` | `query.h` | `query_state<Iter>` + `shared_ptr<Owner>`. **UB pitfall:** see §8. |

| `enumerable<Handle>` | `enumerable.h` | **Private inheritance** from `Handle`; `using Handle::…` exposes members. Lazy ops return `pipe(Next&&)` → `enumerable<std::decay_t<Next>>`. |

---

## 4. Iterator adaptors (`iterators.h`)

| Iterator | Base | Stored state | Category notes |
|----------|------|--------------|----------------|
| `basic_iterator<BaseIt>` | inherits `BaseIt` | none (wrapper) | Same category as `BaseIt`. |
| `select_iterator<BaseIt,F>` | inherits `BaseIt` | `F fn_` | `operator*` → `fn_(*base)`. **Copy-assign:** only updates **base** position; **does not assign `fn_`** (MSVC `std::find_if` assigns iterators; lambdas with ref capture may lack copy-assign). |
| `where_iterator<BaseIt,Pred>` | inherits `BaseIt` | `begin_`, `end_`, `pred_` | Category **clamped** to at most bidirectional (`clamp_iterator_category_t`). `++` skips non-matching. **Copy-assign:** updates base + bounds, **not `pred_`** (same rationale as `select_iterator`). |
| `take_while_iterator` | inherits `BaseIt` | `pred_` | Category clamped; logical `operator==`. |
| `take_n_iterator` | inherits `BaseIt` | `int remaining_` (begin uses negated budget from `take_n_view`) | Category clamped; `++` increments `remaining_` and base. |

**Projection typing:** `select_iterator::reference` uses `detail::projection_reference_t<R>` from `type_traits.h` (preserve lvalue/rvalue ref from projection return type).

---

## 5. `detail::query_range_algorithms` — operation groups

**File:** `query_range.h` (declarations + many inline bodies); **out-of-line** in `query.h` and `extra_views.h` for materializers / join / set ops needing complete `materialized_range` or view types.

### 5.1 Lazy transforms (return new views / `enumerable` via `select_view` etc.)

- `select(F)`, `select_many(Fs...)` → tuple of loader results per element (**not** C# flattening).
- `where(P)`, `of_type<U>()` (pointer + polymorphic + `dynamic_cast`).
- `skip`, `skip_while`, `take`, `take_while`, `reverse`.
- `concat(Rng)`, `zip(Rng)`, `zip_fold(Rng, init, f)` (**single pass**, no `zip_iterator` pairs).
- `default_if_empty`, `default_if_empty(def)`, `enumerate`, `scan(seed,f)`, `chunk(n)`, `stride(step)`.
- `append(T)`, `prepend(T)`, `distinct()`, `distinct_by(kf)`, `union_with(Rng)`.

### 5.2 Relational

- `join(inner, outer_key, inner_key, result_sel)` — inner join.
- `group_join(inner, outer_key, inner_key)` — outer correlated with vectors of inners.
- `to_lookup(key)` → alias **`group_by(key)`**.

### 5.3 Materializers (typically `materialized_range` over `shared_ptr<container>`)

- `group_by(keys...)` — nested maps + leaf `vector` (`group_by.h`).
- `order_by(key_filters...)` — copy to `vector`, `std::sort` with `lexicographic_compare` + `asc`/`desc`/`make_filter`.
- `to_vector()` / `materialize()` — `reserve_if_random_access` + `std::copy` + `back_inserter`.
- `to_unordered_map`, `to_map`, `to_dictionary` (throws on duplicate key).
- `to_set`, `to_unordered_set`.
- `except(Rng)`, `intersect(Rng)` — hash set of RHS; **requires hashable `value_type`**.
- `take_last(n)`, `skip_last(n)` — materialize to `vector` (ring / trim patterns in implementation).

### 5.4 Search / quantifiers

- `contains`, `index_of`, `last_index_of`, `sequence_equal` (+ custom comparator overload).
- `any`, `count`, `long_count`, `long_count_if`.
- Fused: `sum_if`, `count_if`, `any_if`, `all_if`, `none_if` (**one pass**, no `where_iterator`).

### 5.5 Aggregates / stats

- `sum`, `aggregate`/`fold`, `reduce`.
- `average`, `average_if`.
- `min`, `max`, `min_max`, `min_by`, `max_by`, `_or_default` variants.
- `percentile_by` / `percentile`, `median_by` / `median` — materialize keys, sort, interpolate.
- `variance_population_by` / `_sample_by`, Welford single pass; `std_dev_*` → `sqrt`.

### 5.6 Element access terminals

- `first`, `last`, `element_at`, `single` families; many throw `std::out_of_range` with messages prefixed **`qb::linq::`**.

---

## 6. `group_by.h` (nested map shape)

- **Recursive** `group_by_impl<In, KeyFns...>` builds type `group_by_map_t<In, KeyFns...>`.
- **Per level:** `map_type_for_key_t<Key, Value>` → `std::unordered_map` if `Key` is **fundamental**, else **`std::map`**.
- **Leaf:** `std::vector<remove_cvref_t<In>>`.
- **Insertion:** `group_by_impl::emplace(handle, val, keys...)` walks nested maps by `key(val)`.

---

## 7. `order.h` (`order_by` keys)

- `order_kind`: `ascending`, `descending`, `custom`.
- `order_predicate_base<Kind>`: `operator()` strict order; `next` for tie (`==`).
- `order_key_filter<BasePredicate, KeyProj>`: `primary(a,b)` / `tied(a,b)` on projected keys.
- `lexicographic_compare(a, b, filters...)` — multi-key sort comparator driver.
- **Public factories:** `asc(key)`, `desc(key)`; `make_order_filter<CustomBase>(key, args...)` from `enumerable.h` as `make_filter<CustomBase>`.

---

## 8. Critical correctness / UB caveats

1. **`materialized_range` constructor ordering:** Never call the 3-arg ctor as `materialized_range(owner->begin(), owner->end(), std::move(owner))` in **one** full-expression — **unspecified argument eval order** → use **`materialized_range(std::move(owner))`** or `detail::wrap_materialized` (`query.h` comments).

2. **`where_view` thread-safety:** **Not** thread-safe; `first_` cache mutated on first `begin()`.

3. **`zip` / `concat` / algorithms that assign iterators:** Inner functors in nested ranges may need to be **copy-assignable** (e.g. use `+[](){}` function pointer). Related: `select_iterator` / `where_iterator` copy-assign **skips** functor/predicate assignment by design (valid only for iterators from the **same** logical pipeline).

4. **`except` / `intersect`:** `value_type` must be usable in `unordered_set`.

5. **Terminal references:** `first()`, some `decltype(auto)` paths may return **references** into underlying data — same lifetime rules as container/iterator invalidation.

6. **`sum` / `sum_if`:** Require default-constructible accumulator and `operator+=`.

7. **`select_many`:** Semantics ≠ C# `SelectMany`; returns **tuple** of projections.

---

## 9. `extra_views.h` — view / iterator catalog

| Component | Iterator category (typical) | Notable implementation detail |
|-----------|-----------------------------|-------------------------------|
| `empty_view<T>` | forward | Always empty. |
| `single_view<T>` | forward | One element. |
| `repeat_view<T>` | forward | Counted repeats. |
| `concat_view` | **forward** | `concat_iterator`: two legs + `in_first_` flag; `++` branches. |
| `zip_view` | forward | `zip_iterator`; ends at shorter range. |
| `default_if_empty_view` | forward | Yields default once if source empty. |
| `distinct_view` | forward | **`unordered_set` per `begin()`** for seen keys (re-enumeration correct). |
| `enumerate_view` | forward | `(index, *it)` pairs. |
| `chunk_view` | forward | **`chunk_iterator`:** each `++` fills internal **`vector`** chunk (`load_chunk`). |
| `stride_view` | forward (`stride_iterator` advertises forward) | `++` uses RA `+= step` when base is RA, else stepped loop. |
| `scan_view` | forward | **`F` stored in `shared_ptr<F>`**; iterator holds acc + cache for `operator*`. |

**Out-of-line `query_range_algorithms` split:**

- **`query.h`:** `group_by`, `order_by`, `to_vector`, `to_unordered_map`, `to_map`, `to_dictionary`, `except`, `intersect`, `take_last`, `skip_last`, `join`, `group_join`, `to_unordered_set`, `to_set`.
- **`extra_views.h` (tail):** `enumerate`, `scan`, `concat`, `zip`, `default_if_empty(value_type)`, `distinct_by`, `distinct`, `union_with`, `chunk`, `stride`, `append`, `prepend`.

---

## 10. `type_traits.h`

- `iter_reference_t<It>`, `iter_value_t<It>` — `std::iterator_traits` shims.
- `projection_reference_t<R>` — `select` reference type.
- `identity_proj<Ref>` — `distinct()` default key = decayed element.

---

## 11. Free functions & re-exports (`enumerable.h`)

- `from(container&)`, `from(container const&)`, `from(Iter,Iter)`, `range` alias.
- `as_enumerable` overloads (enumerable ref/rvalue, container, iterator pair).
- `iota`, `empty<T>()`, `once`, `repeat`.
- `asc`, `desc`, `make_filter<CustomBase>(...)`.
- CTAD: `enumerable(handle)`.

---

## 12. Portability and implementation contracts

- **Out-of-line member templates:** The definition’s return type and parameter types must match what the class template declares; different spellings of the same type can fail on some compilers. Follow the existing pattern in `query.h` for materializing operations.
- **`select_iterator` / `where_iterator`:** Copy assignment updates iterator position (and `where_iterator` range bounds) but **does not** assign stored callables; this matches the documented behavior in `iterators.h` and interacts with how standard algorithms assign iterators.

---

## 13. Verification hooks (repository)

- **Tests:** `tests/*.cpp` (GoogleTest); CMake `QB_BUILD_TESTS`.
- **Benchmarks:** `benchmarks/*.cpp` (Google Benchmark); `QB_BUILD_BENCHMARKS` → `qb_linq_benchmark`.
- **Human README:** performance expectations — root `README.md` section **Performance & benchmarks**.

---

## 14. How to use this document (for LLMs)

- When adding an operation: locate **declaration** in `query_range.h`, **definition** in `query.h` or `extra_views.h`, and any **iterator** in `iterators.h` or `extra_views.h`.
- When a compiler rejects an out-of-line definition: verify it **matches** the in-class declaration (dependent type names, reference type spelling).
- When answering API questions: prefer **behavior described here + Doxygen in `qb/linq.h`** over guessing C# LINQ parity.

**Last reviewed against source layout:** repository file paths as under `include/qb/linq/`; line-level details may drift — grep is authoritative.
