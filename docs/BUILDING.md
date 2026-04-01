# Building and installing qb-linq

**Index:** all guides are listed in **[`README.md`](README.md)** (this folder).

This document matches the **root [`CMakeLists.txt`](../CMakeLists.txt)**, **[`CMakePresets.json`](../CMakePresets.json)**, and the **`tests/`** / **`benchmarks/`** CMake files. The user-facing summary lives in the root **[`README.md`](../README.md)** (Build options, presets). **CI** runs a **`docs`** job (Ubuntu + Doxygen) so **`docs/Doxyfile.in`** stays valid — see **[`.github/workflows/cmake.yml`](../.github/workflows/cmake.yml)**.

## Project (CMake facts)

| Item | Value |
|------|--------|
| **CMake project** | `qb-linq` |
| **Version** | `1.2.1` (`project(... VERSION 1.2.1)`) |
| **Library type** | **INTERFACE** — no compiled `.lib` / `.a`; headers only |
| **CMake target** | `qb::linq` (alias of `qb-linq`) |
| **Include interface** | `include/` plus **`generated_include/`** (build tree) — holds **`qb/linq/version.h`** generated from **`project(VERSION)`** |
| **C++ standard** | **C++17** (`cxx_std_17` on the interface target) |
| **Minimum CMake** | **3.22** |

**Versioning:** See **[`docs/VERSIONING.md`](VERSIONING.md)** — bump **`project(qb-linq VERSION …)`** only; **`version.h`** is produced at configure time from **`cmake/version.h.in`**.

## Consuming the library

- **Installed package:** `find_package(qb-linq 1.2 CONFIG REQUIRED)` then `target_link_libraries(... PRIVATE qb::linq)` (see README).
- **In-tree / vendor:** `add_subdirectory(qb-linq)` then `target_link_libraries(... PRIVATE qb::linq)`.
- **Include in C++:** `#include <qb/linq.h>` only (per README).

**Install layout** (from `CMakeLists.txt`): `include/` copied to `${CMAKE_INSTALL_INCLUDEDIR}`; CMake package files under `${CMAKE_INSTALL_LIBDIR}/cmake/qb-linq` (`qb-linq-config.cmake`, version file, exported target `qb::linq`).

## Options

| CMake option | Default | Effect |
|--------------|---------|--------|
| **`QB_BUILD_TESTS`** | **ON** if **`PROJECT_IS_TOP_LEVEL`**, else **OFF** | Adds **`tests/`** → executable **`qb_linq_tests`**, **`enable_testing()`**, GoogleTest **`gtest_discover_tests`** |
| **`QB_BUILD_BENCHMARKS`** | **OFF** | Adds **`benchmarks/`** → executable **`qb_linq_benchmark`** |
| **`QB_LINQ_BUILD_DOCS`** | **OFF** | If **ON** and Doxygen is found: configures **`docs/Doxyfile.in`** → custom target **`qb_linq_docs`** (HTML under `${CMAKE_BINARY_DIR}/doc_doxygen/html`). Warnings are logged to **`${CMAKE_BINARY_DIR}/doxygen-qb-linq.warnings.log`**; **CI** fails if that file is non-empty after building **`qb_linq_docs`**. |

When qb-linq is added as a **subproject**, tests are **off** unless you pass **`-DQB_BUILD_TESTS=ON`**.

## Presets (`CMakePresets.json`)

| Preset | `CMAKE_BUILD_TYPE` | Tests | Benchmarks | Docs |
|--------|-------------------|-------|------------|------|
| **`release`** | Release | OFF | OFF | OFF |
| **`dev`** | RelWithDebInfo | ON | ON | OFF |
| **`ci`** | Release | ON | ON | OFF |
| **`docs`** | Release | OFF | OFF | ON (needs Doxygen on `PATH`) |

**Binary directory:** `build/<presetName>/` (e.g. `build/dev/`).

Examples:

```bash
cmake --preset dev
cmake --build build/dev
ctest --preset dev
```

On Windows, the benchmark binary is typically `build/dev/benchmarks/qb_linq_benchmark.exe` (path varies with generator and preset).

## Test and benchmark dependencies

- **Tests:** [GoogleTest](https://github.com/google/googletest) via **FetchContent**, tag **`v1.14.0`** (`tests/CMakeLists.txt`).
- **Benchmarks:** [Google Benchmark](https://github.com/google/benchmark) via **FetchContent**, tag **`v1.9.1`** (`benchmarks/CMakeLists.txt`).

Both disable install of the fetched projects where configured in those files.

## Compiler flags (targets in this repo)

- **`qb_linq_tests`:** MSVC → `/W4 /permissive-`; else → `-Wall -Wextra -Wpedantic`.
- **`qb_linq_benchmark`:** extra `-Wall -Wextra` on non-MSVC (`benchmarks/CMakeLists.txt`).

## Top-level developer workflow

1. Configure with tests (and optionally benchmarks): e.g. **`cmake --preset dev`**.
2. Build **`qb_linq_tests`** (and **`qb_linq_benchmark`** if enabled).
3. Run **`ctest --preset dev`** or execute `qb_linq_tests` directly.

Behaviour changes should keep **`QB_BUILD_TESTS=ON`** green; changes near hot paths should keep **`QB_BUILD_BENCHMARKS=ON`** builds green when benchmarks are enabled in CI or locally.

## Single-header amalgamation

| Custom target | Effect |
|---------------|--------|
| **`qb_linq_single_header`** | Rewrites **`single_header/linq.h`** from **`include/qb/linq/*.h`** + embedded **`version.h`**. CMake prefers **Python 3** on Unix (or when **FindPython3** finds it); on Windows without Python it uses **PowerShell**. |

From the **repository root**, you can regenerate manually with any of these (same result):

| Script | Typical use |
|--------|-------------|
| **`scripts/amalgamate_single_header.py`** | **`python3 …`** or **`python …`** — any OS with Python 3. |
| **`scripts/amalgamate_single_header.sh`** | **`sh …`** — POSIX; runs **`python3`**. |
| **`scripts/amalgamate_single_header.ps1`** | **`pwsh -File …`** — PowerShell (often Windows). |

After changing library headers or **`project(VERSION …)`**, rebuild **`qb_linq_single_header`** (or run a script) and commit **`single_header/linq.h`**. CI runs **`qb_linq_single_header_test`** (include path **`single_header/`** only).
