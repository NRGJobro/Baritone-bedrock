#!/usr/bin/env python3
"""Report SDK files that are unreachable from the client's real translation units.

The SDK is intentionally treated as an implementation library rather than a bag
of roots: a SDK .cpp only becomes live when its matching header is reachable
from a non-SDK source compiled by CMake (or from another live SDK file).
"""
from __future__ import annotations

import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
SDK = SRC / "SDK"
CMAKE = ROOT / "CMakeLists.txt"

SOURCE_SUFFIXES = {".h", ".hpp", ".hh", ".cpp", ".cc", ".cxx"}
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.MULTILINE)
CMAKE_SOURCE_RE = re.compile(r"^\s*(src/\S+\.(?:cpp|cc|cxx))\s*$", re.MULTILINE)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def resolve_include(owner: Path, include: str, known: set[Path]) -> Path | None:
    include = include.replace("\\", "/")
    candidates = [
        (owner.parent / include).resolve(),
        (ROOT / include).resolve(),
        (SRC / include).resolve(),
        (SDK / include).resolve(),
        (ROOT / "include" / include).resolve(),
    ]
    for candidate in candidates:
        if candidate in known:
            return candidate
    return None


def companion_cpp(header: Path, known: set[Path]) -> Path | None:
    if header.suffix.lower() not in {".h", ".hpp", ".hh"}:
        return None
    for suffix in (".cpp", ".cc", ".cxx"):
        candidate = header.with_suffix(suffix)
        if candidate in known:
            return candidate
    return None


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--machine", action="store_true", help="print only unused repo-relative paths")
    args = parser.parse_args()

    files = {
        p.resolve()
        for p in SRC.rglob("*")
        if p.is_file() and p.suffix.lower() in SOURCE_SUFFIXES
    }
    files.add((ROOT / "PCH.h").resolve())

    graph: dict[Path, set[Path]] = {}
    for path in files:
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        deps: set[Path] = set()
        for include in INCLUDE_RE.findall(text):
            resolved = resolve_include(path, include, files)
            if resolved is not None:
                deps.add(resolved)
        graph[path] = deps

    cmake_text = CMAKE.read_text(encoding="utf-8")
    roots: set[Path] = {(ROOT / "PCH.h").resolve()}
    for item in CMAKE_SOURCE_RE.findall(cmake_text):
        path = (ROOT / item).resolve()
        # SDK implementations are pulled in only when their public header is
        # actually reachable. Treating every current SDK .cpp as a root would
        # make dead SDK code impossible to discover.
        if path.exists() and not path.is_relative_to(SDK.resolve()):
            roots.add(path)

    reachable: set[Path] = set()
    queue = list(roots)
    while queue:
        current = queue.pop()
        if current in reachable:
            continue
        reachable.add(current)
        queue.extend(graph.get(current, ()))

        # An SDK wrapper's out-of-line implementation is live whenever its
        # matching public header is live.
        if current.is_relative_to(SDK.resolve()):
            impl = companion_cpp(current, files)
            if impl is not None and impl not in reachable:
                queue.append(impl)

    sdk_files = sorted(p for p in files if p.is_relative_to(SDK.resolve()))
    unused = [p for p in sdk_files if p not in reachable]

    if args.machine:
        for path in unused:
            print(rel(path))
        return 0

    live = [p for p in sdk_files if p in reachable]
    print(f"SDK source/header files: {len(sdk_files)}")
    print(f"Reachable from Limiter build: {len(live)}")
    print(f"Unused SDK candidates: {len(unused)}")
    print("\nUNUSED_SDK_BEGIN")
    for path in unused:
        print(rel(path))
    print("UNUSED_SDK_END")
    print("\nLIVE_SDK_BEGIN")
    for path in live:
        print(rel(path))
    print("LIVE_SDK_END")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
