#!/usr/bin/env python3
"""Count the C test sources whose checks are assert() calls (they compile to nothing under NDEBUG).

A C test is a `tests/**/*.c` file. It is assert-based when it includes <assert.h> and calls assert( outside comments.
The CMake gate (-UNDEBUG on every test target, plus poolrad_assert_canary_tests) keeps these live; this script makes
the number reproducible instead of quoted.

    count_assert_tests.py            print '<assert-based> of <total> C test files'
    count_assert_tests.py --list     also list the assert-based files
"""
# NOT_A_ROW_CHECKER: counts assert()-based C test files for the NDEBUG gate; it checks no registered row
import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INCLUDE = re.compile(r"^\s*#\s*include\s*<assert\.h>", re.M)
CALL = re.compile(r"\bassert\s*\(")


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def assert_based(path):
    text = strip_comments(Path(path).read_text(errors="replace"))
    return bool(INCLUDE.search(text) and CALL.search(text))


def census(root=ROOT):
    files = sorted(p for p in (Path(root) / "tests").rglob("*.c"))
    return files, [p for p in files if assert_based(p)]


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--list", action="store_true")
    args = parser.parse_args(argv)
    files, based = census()
    print(f"{len(based)} of {len(files)} C test files are assert()-based")
    if args.list:
        for path in based:
            print(path.relative_to(ROOT))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
