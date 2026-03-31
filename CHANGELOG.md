# Changelog

All notable changes to **qb-linq** are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html). The canonical version is **`project(qb-linq VERSION …)`** in `CMakeLists.txt` — see [`docs/VERSIONING.md`](docs/VERSIONING.md).

## [Unreleased]

### Added

- _(nothing yet)_

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

[1.1.0]: https://github.com/isndev/qb-linq/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/isndev/qb-linq/releases/tag/v1.0.0
