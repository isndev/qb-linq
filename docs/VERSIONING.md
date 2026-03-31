# Versioning (qb-linq)

## Single source of truth

The library version is **`project(qb-linq VERSION x.y.z)`** in the root **[`CMakeLists.txt`](../CMakeLists.txt)**.

At configure time, CMake generates **`qb/linq/version.h`** into the build tree (`generated_include/`). That file is:

- Visible to targets that link **`qb::linq`** (extra include path), and  
- **Installed** next to the other headers under `include/qb/linq/version.h`.

**`<qb/linq.h>`** includes **`version.h`**, so you always see the same version as the CMake package.

The optional drop-in **`single_header/linq.h`** embeds the same version macros (regenerate with target **`qb_linq_single_header`** after changing **`project(VERSION …)`**).

## Semantic versioning

Versions follow **SemVer** (`MAJOR.MINOR.PATCH`):

- **MAJOR** — incompatible API changes (CMake package uses **`SameMajorVersion`** compatibility in `qb-linq-config-version.cmake`).
- **MINOR** — backward-compatible additions.
- **PATCH** — backward-compatible fixes.

## C++ API

| Symbol | Meaning |
|--------|---------|
| **`QB_LINQ_VERSION_MAJOR`**, **`MINOR`**, **`PATCH`** | Preprocessor integers |
| **`QB_LINQ_VERSION_STRING`** | String literal, e.g. `"1.2.0"` |
| **`QB_LINQ_VERSION_INTEGER`** | `major*1'000'000 + minor*1'000 + patch` for `#if` (minor/patch &lt; 1000) |
| **`qb::linq::version`** | `constexpr` struct mirroring the macros |

Example:

```cpp
#include <qb/linq.h>
#include <iostream>

#if QB_LINQ_VERSION_INTEGER >= 1000000
#  define AT_LEAST_1_0
#endif

int main() {
    std::cout << QB_LINQ_VERSION_STRING << '\n';
    static_assert(qb::linq::version::major == QB_LINQ_VERSION_MAJOR, "");
}
```

## CMake

```cmake
find_package(qb-linq 1.2 CONFIG REQUIRED)
# Package version must satisfy SameMajorVersion vs the installed qb-linq.
```

## Releases

1. Bump **`project(... VERSION x.y.z)`** in **`CMakeLists.txt`**.
2. Reconfigure CMake (regenerates **`version.h`**).
3. Update **[`CHANGELOG.md`](../CHANGELOG.md)** — move **`[Unreleased]`** items into a new **`[x.y.z] - YYYY-MM-DD`** section.
4. Tag the repo (**`v1.2.0`**, etc.) and publish GitHub release notes (can mirror the changelog entry).

## Vendoring without CMake

If you copy only **`include/`** and never run CMake, **`qb/linq/version.h` will be missing** and **`#include <qb/linq.h>`** will fail. Options:

- Run **`cmake -B build`** once and copy **`build/.../generated_include/qb/linq/version.h`** into your tree, or  
- Prefer **`add_subdirectory`** / **`find_package`** so generation stays automatic.
