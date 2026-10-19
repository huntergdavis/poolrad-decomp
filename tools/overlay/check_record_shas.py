#!/usr/bin/env python3
"""Fail when a 40-hex commit id in the accounting record is not a real commit.

The reviewer lane once typed a plausible-looking but invented merge SHA into a
reviewed-identity row; the CSV validator only checks the *shape* (40 hex), so it
would have landed. This check closes that hole mechanically: every 40-hex token
in the accounting record must resolve via ``git cat-file -e <sha>^{commit}``,
unless the same line marks it explicitly as external/unknown provenance.

Exit status: 0 when every token resolves or is marked, 1 otherwise.
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

# Files that carry provenance we credit: ledger rows, acceptance record, model
# headers. 64-hex body digests are content hashes and are intentionally ignored.
TARGETS = (
    "docs/re/strict_full_c_addresses.csv",
    "docs/re/reviewed_function_identities.csv",
    "docs/re/partial_callee_models.csv",
    "docs/re/resident_callable_ledger.csv",
    "docs/re/function_accounting_checkpoint.json",
)
SHA40 = re.compile(r"(?<![0-9a-fA-F])[0-9a-f]{40}(?![0-9a-fA-F])")
# An explicit marker lets a genuinely external id stay in the record.
MARKERS = re.compile(r"\b(external|unknown|not[-_ ]in[-_ ]repo|upstream|unresolved)\b",
                     re.IGNORECASE)


def commits(root):
    out = subprocess.run(["git", "-C", str(root), "rev-list", "--all"],
                         capture_output=True, text=True, check=True)
    return set(out.stdout.split())


def scan(root, known):
    """Return problems plus counts for the record files under ``root``."""
    problems, checked, marked = [], 0, 0
    for rel in TARGETS:
        path = Path(root) / rel
        if not path.is_file():
            continue
        for number, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
            for token in SHA40.findall(line):
                checked += 1
                if token in known:
                    continue
                if MARKERS.search(line):
                    marked += 1
                    continue
                problems.append(f"{rel}:{number}: {token} is not a commit in this repo")
    return problems, checked, marked


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=".", help="repository root")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)
    root = Path(args.root).resolve()
    known = commits(root)
    problems, checked, marked = scan(root, known)
    if not args.quiet:
        print(f"checked {checked} 40-hex ids; {marked} marked external/unknown")
    for problem in problems:
        print(problem, file=sys.stderr)
    if problems:
        print(f"FAIL: {len(problems)} unbacked commit id(s)", file=sys.stderr)
        return 1
    print("OK: every 40-hex id resolves or is explicitly marked")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
