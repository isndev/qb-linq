#!/usr/bin/env python3
"""
Amalgamate qb-linq into single_header/linq.h (same API as #include <qb/linq.h>).

Run from repository root:
  python scripts/amalgamate_single_header.py

Or: cmake --build <build> --target qb_linq_single_header
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
LINQ_INCLUDE = REPO / "include" / "qb" / "linq"

# Dependency order (leaves first).
ORDER = [
    "detail/type_traits.h",
    "detail/order.h",
    "detail/group_by.h",
    "detail/lazy_copy_once.h",
    "detail/query_range.h",
    "iterators.h",
    "reverse_iterator.h",
    "detail/extra_views.h",
    "query.h",
    "enumerable.h",
]


def parse_cmake_version(cmakelists: Path) -> tuple[int, int, int, str]:
    text = cmakelists.read_text(encoding="utf-8")
    m = re.search(r"project\s*\(\s*qb-linq\s+VERSION\s+(\d+)\.(\d+)\.(\d+)", text, re.I)
    if not m:
        sys.exit("Could not parse project(qb-linq VERSION x.y.z) in CMakeLists.txt")
    major, minor, patch = int(m.group(1)), int(m.group(2)), int(m.group(3))
    return major, minor, patch, f"{major}.{minor}.{patch}"


def render_version_h(template: Path, major: int, minor: int, patch: int, ver: str) -> str:
    s = template.read_text(encoding="utf-8")
    s = s.replace("@PROJECT_VERSION_MAJOR@", str(major))
    s = s.replace("@PROJECT_VERSION_MINOR@", str(minor))
    s = s.replace("@PROJECT_VERSION_PATCH@", str(patch))
    s = s.replace("@PROJECT_VERSION@", ver)
    return s


def strip_for_amalgam(source: str) -> tuple[list[str], str]:
    """Split out #include <...> lines; drop #pragma once and #include \"qb/linq/...\"."""
    sys_includes: list[str] = []
    seen: set[str] = set()
    out: list[str] = []
    for line in source.splitlines(keepends=True):
        t = line.strip()
        if t.startswith("#include"):
            rest = t[len("#include") :].strip()
            if rest.startswith('"qb/linq'):
                continue
            if rest.startswith("<") and rest.endswith(">"):
                key = rest
                if key not in seen:
                    seen.add(key)
                    sys_includes.append(line.rstrip("\r\n"))
                continue
            if rest.startswith('"'):
                continue
        if t == "#pragma once":
            continue
        out.append(line)
    body = "".join(out).rstrip() + ("\n" if out else "")
    return sys_includes, body


def merge_includes(chunks: list[list[str]]) -> list[str]:
    seen: set[str] = set()
    ordered: list[str] = []
    for group in chunks:
        for line in group:
            key = line.strip()
            if key not in seen:
                seen.add(key)
                ordered.append(line)
    return sorted(ordered, key=str.lower)


def main() -> None:
    major, minor, patch, ver = parse_cmake_version(REPO / "CMakeLists.txt")
    version_src = render_version_h(REPO / "cmake" / "version.h.in", major, minor, patch, ver)
    v_includes, version_body = strip_for_amalgam(version_src)

    include_groups: list[list[str]] = [v_includes]
    parts: list[tuple[str, str]] = []

    for rel in ORDER:
        path = LINQ_INCLUDE / rel
        if not path.is_file():
            sys.exit(f"Missing header: {path}")
        raw = path.read_text(encoding="utf-8")
        inc, body = strip_for_amalgam(raw)
        include_groups.append(inc)
        parts.append((rel, body))

    all_includes = merge_includes(include_groups)

    lines: list[str] = [
        "/**",
        " * @file linq.h",
        " * @brief Single-header qb-linq (same public API as `#include <qb/linq.h>`).",
        " *",
        " * @note Do not edit by hand. Regenerate with:",
        " * `python scripts/amalgamate_single_header.py` or CMake target **qb_linq_single_header**.",
        f" * @note Embedded version **{ver}** from `CMakeLists.txt` `project(VERSION ...)`.",
        " */",
        "",
        "#pragma once",
        "",
    ]
    lines.extend(all_includes)
    lines.append("")
    lines.append("// ---- qb/linq/version.h (embedded) ----")
    lines.append(version_body.rstrip())
    lines.append("")

    for rel, body in parts:
        lines.append(f"// ---- qb/linq/{rel} ----")
        lines.append(body.rstrip())
        lines.append("")

    out_path = REPO / "single_header" / "linq.h"
    out_path.parent.mkdir(parents=True, exist_ok=True)
    text = "\n".join(lines).rstrip() + "\n"
    out_path.write_text(text, encoding="utf-8", newline="\n")
    print(f"Wrote {out_path} ({len(text.encode('utf-8'))} bytes)")


if __name__ == "__main__":
    main()
