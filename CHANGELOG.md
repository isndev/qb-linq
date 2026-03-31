# Changelog

All notable changes to **qb-linq** are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html). The canonical version is **`project(qb-linq VERSION …)`** in `CMakeLists.txt` — see [`docs/VERSIONING.md`](docs/VERSIONING.md).

## [Unreleased]

### Added

- **[`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md)** (Contributor Covenant 2.1).
- **Dependabot** for GitHub Actions (`.github/dependabot.yml`).
- Issue chooser **contact links** (security, CoC, contributing).

### Changed

- **CI `docs` job** fails if Doxygen writes a non-empty **`doxygen-qb-linq.warnings.log`**.
- **Doxygen** strips both `include/` and the generated **`version.h`** include root for shorter file paths in HTML.

### Fixed

- **`AGENTS.md`** no longer hardcodes a library version number (use **`CMakeLists.txt`** / **`docs/VERSIONING.md`**).
- **`.gitignore`** documents **`doc_doxygen/`** and related outputs; keeps legacy **`doc/`** ignores.

## [1.0.0] - 2026-03-31

### Added

- Header-only **`qb::linq`** API: lazy LINQ-style pipelines for **C++17** (`#include <qb/linq.h>`).
- CMake **INTERFACE** target **`qb::linq`**, install + **`find_package(qb-linq CONFIG)`** (`SameMajorVersion`).
- Generated **`qb/linq/version.h`** from CMake `project(VERSION)` (macros + `qb::linq::version`).
- GoogleTest suite (**`qb_linq_tests`**) and Google Benchmark suite (**`qb_linq_benchmark`**).
- Doxygen HTML docs (**`qb_linq_docs`**, Doxygen Awesome under `docs/doxygen/`).
- Guides: building, versioning, LLM/agent context; **CONTRIBUTING**, **SECURITY**, GitHub templates.
