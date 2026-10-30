#!/usr/bin/env python3
"""Fail when a 40-hex commit id in the accounting record is not a real commit.

The reviewer lane once typed a plausible-looking but invented merge SHA into a
reviewed-identity row; the CSV validator only checks the *shape* (40 hex), so it
would have landed. This check closes that hole mechanically: every 40-hex token
in the accounting record must resolve via ``git cat-file -e <sha>^{commit}``,
unless the same line marks it explicitly as external/unknown provenance.

Exit status: 0 when every token resolves or is marked, 1 otherwise.

A SHALLOW CLONE is not a failing repo (t675). `commits()` reads `git rev-list
--all`, which in a shallow clone lists only the history that was fetched, so base
commits and upstream ids are absent and the guard used to report them as invented.
Four body PRs on 2026-10-29/30 copied that CI failure into their bodies as "fails
identically at the base", which was false for anyone reproducing in a full clone.
The guard now separates the two: absent in a shallow clone AND absent as an object
means NOT VERIFIABLE HERE (reported, not failed); absent in a full clone is still a
fabricated id and still exits 1.
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


def is_shallow(root):
    """True when this checkout has truncated history (a CI `--depth` clone)."""
    try:
        out = subprocess.run(["git", "-C", str(root), "rev-parse",
                              "--is-shallow-repository"],
                             capture_output=True, text=True, timeout=30)
    except (OSError, subprocess.SubprocessError):
        return False
    return out.returncode == 0 and out.stdout.strip() == "true"


def object_exists(root, sha):
    """True when the commit object itself is present, reachable or not."""
    try:
        out = subprocess.run(["git", "-C", str(root), "cat-file", "-e",
                              f"{sha}^{{commit}}"],
                             capture_output=True, text=True, timeout=30)
    except (OSError, subprocess.SubprocessError):
        return False
    return out.returncode == 0


def scan(root, known, shallow=None, exists=None):
    """(problems, checked, marked, unverifiable) for the record files under ``root``.

    `unverifiable` holds the tokens that are absent from a SHALLOW clone's history
    and also absent as objects: this clone cannot decide whether they are real, so
    they are reported and not failed (t675). `shallow` and `exists` are injectable
    so a test never needs a real shallow clone.
    """
    if shallow is None:
        shallow = is_shallow(root)
    check_object = exists if exists is not None else (lambda sha: object_exists(root, sha))
    problems, checked, marked = [], 0, 0
    unverifiable = []
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
                if shallow and not check_object(token):
                    unverifiable.append(f"{rel}:{number}: {token}")
                    continue
                problems.append(f"{rel}:{number}: {token} is not a commit in this repo")
    return problems, checked, marked, unverifiable


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=".", help="repository root")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)
    root = Path(args.root).resolve()
    known = commits(root)
    problems, checked, marked, unverifiable = scan(root, known)
    shallow = is_shallow(root)
    if not args.quiet:
        print(f"checked {checked} 40-hex ids; {marked} marked external/unknown")
        if unverifiable:
            print(f"NOT VERIFIABLE HERE: {len(unverifiable)} id(s) are absent from "
                  "this SHALLOW clone's history and are not present as objects; "
                  "fetch the full history (git fetch --unshallow) to check them:")
    for entry in unverifiable:
        print(entry, file=sys.stderr)
    for problem in problems:
        print(problem, file=sys.stderr)
    if problems:
        print(f"FAIL: {len(problems)} unbacked commit id(s)", file=sys.stderr)
        return 1
    suffix = ""
    if unverifiable:
        suffix = (f"; {len(unverifiable)} unverifiable in a shallow clone "
                  f"({'shallow' if shallow else 'full'} history)")
    print("OK: every 40-hex id resolves or is explicitly marked" + suffix)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
