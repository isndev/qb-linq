# Contributing to qb-linq

Thanks for your interest. This project is a **header-only C++17** library; all library code lives under **`include/qb/linq/`**.

## Before you start

1. This project follows the **[Contributor Covenant](CODE_OF_CONDUCT.md)** — be respectful in issues and PRs.
2. Read **[`docs/BUILDING.md`](docs/BUILDING.md)** — configure, build, and run tests locally.
3. Skim **[`include/qb/linq.h`](include/qb/linq.h)** — Doxygen **`linq`** group lists API caveats (`select_many`, `zip`/`concat`, threading, etc.).
4. For non-trivial code changes, **[`docs/LLM_CONTEXT.md`](docs/LLM_CONTEXT.md)** maps declarations vs definitions across headers.

## Workflow

1. **Fork** and create a **branch** from `main` (or use a fork’s default workflow your team prefers).
2. **Make focused changes** — match existing naming (`snake_case` API), Doxygen style, and patterns in nearby code.
3. **Build and test**
   - `cmake --preset dev` (or equivalent: tests + benchmarks on).
   - `cmake --build build/dev`
   - `ctest --preset dev` (or `ctest --test-dir build/dev --output-on-failure`).
4. If you touch performance-sensitive code paths, keep **`QB_BUILD_BENCHMARKS=ON`** builds **green** (preset **dev** / **ci**).
5. **Open a PR** against `main` — use the PR template when shown.

## Requirements for PRs

| Change type | Expectation |
|-------------|-------------|
| **Behaviour** | New or updated cases in **`tests/*.cpp`** (GoogleTest). |
| **Docs only** | No tests required unless examples were wrong or misleading. |
| **Public API** | Update **[`README.md`](README.md)** API tables / examples if needed; note **[`CHANGELOG.md`](CHANGELOG.md)** under **`[Unreleased]`**. |
| **Headers under `include/qb/linq/`** | Regenerate **`single_header/linq.h`** (`cmake --build … --target qb_linq_single_header`) and commit it. |
| **Version bump** | Usually done by maintainers: **`CMakeLists.txt`** `project(VERSION …)`, regenerate / verify **`version.h`**, update **CHANGELOG** release section. |

## Commits

- Use **clear English** messages (full sentences when possible).
- Describe **what** changed and **why**; reference issues as `Fixes #123` when applicable.

## AI-assisted contributions

If you use coding agents, follow **[`AGENTS.md`](AGENTS.md)** so changes stay aligned with the repo’s layout and constraints.

## Security

Do **not** open a public issue for security vulnerabilities. See **[`SECURITY.md`](SECURITY.md)**.

## License

By contributing, you agree that your contributions are licensed under the same terms as the project (**[MIT](LICENSE)**).
