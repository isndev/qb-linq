# qb-linq documentation

This folder holds **project guides** and **Doxygen** assets. The main user-facing overview and API tables are in the root **[`README.md`](../README.md)**.

| Document | Audience | Purpose |
|----------|----------|---------|
| **[`BUILDING.md`](BUILDING.md)** | Developers | CMake options, presets, targets (`qb_linq_tests`, `qb_linq_benchmark`, `qb_linq_docs`), FetchContent deps |
| **[`VERSIONING.md`](VERSIONING.md)** | Maintainers / users | SemVer, `version.h`, `find_package`, releases |
| **[`LLM_CONTEXT.md`](LLM_CONTEXT.md)** | Agents / advanced contributors | Implementation map: headers, CRTP, out-of-line split, contracts |
| **Doxygen** (`Doxyfile.in`, `doxygen/`) | Everyone | HTML API reference: `cmake --preset docs` or `-DQB_LINQ_BUILD_DOCS=ON`, target **`qb_linq_docs`** → `${CMAKE_BINARY_DIR}/doc_doxygen/html`. **CI** runs **`docs`** (Ubuntu + Doxygen) and **`sanitize`** (Ubuntu + ASan/UBSan on tests) — see **[`.github/workflows/cmake.yml`](../.github/workflows/cmake.yml)**. |

## Repository-level files

| File | Purpose |
|------|---------|
| **[`../README.md`](../README.md)** | Features, quick start, full API reference, examples |
| **[`../CONTRIBUTING.md`](../CONTRIBUTING.md)** | How to propose changes, tests, PR checklist |
| **[`../CODE_OF_CONDUCT.md`](../CODE_OF_CONDUCT.md)** | Community standards ([Contributor Covenant](https://www.contributor-covenant.org/)) |
| **[`../CHANGELOG.md`](../CHANGELOG.md)** | Release history ([Keep a Changelog](https://keepachangelog.com/)) |
| **[`../SECURITY.md`](../SECURITY.md)** | How to report security issues |
| **[`../AGENTS.md`](../AGENTS.md)** | Checklist for AI coding agents |

## Doxygen layout

- **`Doxyfile.in`** — configured by CMake (`@PROJECT_VERSION@`, paths under `docs/doxygen/`).
- **`doxygen/theme/`** — Doxygen Awesome theme and extensions.
- **`doxygen/header.html`**, **`custom.css`**, **`logo.svg`** — site chrome.
