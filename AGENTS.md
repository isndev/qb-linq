# Instructions for AI coding agents — qb-linq

## What this repository is

**qb-linq** is a **header-only** C++ library (CMake **INTERFACE** target **`qb::linq`**). The numeric version lives in **`project(qb-linq VERSION …)`** in `CMakeLists.txt` (see the table below). Implementation is entirely under **`include/`**; there is no separate library `.cpp` to compile for consumers.

| Fact | Source |
|------|--------|
| Public API | `#include <qb/linq.h>`, namespace **`qb::linq`** |
| CMake target | **`qb::linq`** |
| C++ standard | **17** |
| Library version | **`project(qb-linq VERSION …)`** in `CMakeLists.txt` → generated **`qb/linq/version.h`** — **[`docs/VERSIONING.md`](docs/VERSIONING.md)** |
| Layout | See README **Repository layout** and **[`docs/LLM_CONTEXT.md`](docs/LLM_CONTEXT.md)**; optional amalgam **`single_header/linq.h`** (regenerate **`qb_linq_single_header`**) |

**Build, presets, options, install:** **[`docs/BUILDING.md`](docs/BUILDING.md)** (aligned with root `CMakeLists.txt` and `CMakePresets.json`).

## Before changing code

1. **[`docs/LLM_CONTEXT.md`](docs/LLM_CONTEXT.md)** — include graph, handle types, where declarations vs definitions live, materialization / iterator contracts.
2. **[`include/qb/linq.h`](include/qb/linq.h)** — Doxygen **`@defgroup linq`**: caveats (`select_many`, `zip`/`concat`, `where_view` cache, hashing for set ops, thread-safety, etc.).
3. **Minimal diffs** — match existing style, naming, and Doxygen; no unrelated refactors.

## Where implementation lives

| Task | Primary files |
|------|----------------|
| Algorithm declarations (`query_range_algorithms`) | `include/qb/linq/detail/query_range.h` |
| Out-of-line bodies (many materializers, join, …) | `include/qb/linq/query.h` **or** `include/qb/linq/detail/extra_views.h` — split table in **`docs/LLM_CONTEXT.md`** |
| Iterator adaptors (`select`, `where`, `take`, …) | `include/qb/linq/iterators.h` |
| Compositional views (`concat`, `zip`, `scan`, `chunk`, …) | `include/qb/linq/detail/extra_views.h` |
| `group_by` container shape | `include/qb/linq/detail/group_by.h` |
| `order_by` keys / comparators | `include/qb/linq/detail/order.h` |
| Public façade and factories | `include/qb/linq/enumerable.h` |
| Umbrella include | `include/qb/linq.h` → `version.h` (generated) + `enumerable.h` |

Downstream code should depend only on **`<qb/linq.h>`**.

## Verify your change

Follow **`docs/BUILDING.md`**. In short: enable **`QB_BUILD_TESTS`**, build **`qb_linq_tests`**, run **`ctest`**. When touching performance-sensitive code, also configure with **`QB_BUILD_BENCHMARKS=ON`** and ensure **`qb_linq_benchmark`** still builds.

Agents should **run** builds/tests when the environment allows, not only suggest commands.

## Portability

README and CI target **GCC, Clang, MSVC** on **Linux, macOS, Windows**. Template and out-of-line member definitions must remain valid on all of them; signatures must **match** in-class declarations.

## Tests

- Framework: **GoogleTest**; sources in **`tests/*.cpp`** (`linq_*_test.cpp` pattern).
- Behaviour changes need **tests** unless the change is documentation-only.

## Other docs

| Doc | Role |
|-----|------|
| [`README.md`](README.md) | Users, full API tables, examples |
| [`docs/README.md`](docs/README.md) | Index of guides under `docs/` |
| [`docs/BUILDING.md`](docs/BUILDING.md) | CMake options, presets, targets |
| [`docs/VERSIONING.md`](docs/VERSIONING.md) | SemVer, releases, `version.h` |
| [`docs/LLM_CONTEXT.md`](docs/LLM_CONTEXT.md) | Dense implementation map for tools |
| [`CHANGELOG.md`](CHANGELOG.md) | Release history |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | Human contributor workflow |
| [`SECURITY.md`](SECURITY.md) | Security reporting |
| [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md) | Community standards |
| [`.cursor/rules/`](.cursor/rules/) | Cursor project rules |
| [`.cursor/skills/qb-linq-development/SKILL.md`](.cursor/skills/qb-linq-development/SKILL.md) | Cursor project skill |

## Commits / PRs

Clear **English** messages; state **what** and **why**; link issues when relevant.
