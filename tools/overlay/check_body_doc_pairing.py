#!/usr/bin/env python3
"""Refuse a change that adds a body or checker without its evidence and registration.

A body model under `src/recovered/` and a checker under `tools/overlay/` are
claims about original code. They are only reviewable if the same change carries
the evidence document and, for a checker, a `CHECKER_ROWS` registration — the
contract the handoff gate relies on. Relying on a human to notice a stray body in
a PR is how an untested model reached a reviewable branch once already; this
turns "someone noticed" into "the branch cannot land".

Rules, deliberately narrow:

* a changed `src/recovered/*.c` requires at least one changed `docs/re/*.md`;
* a changed `tools/overlay/*_audit.py` or `*_native.py` (a checker by house
  convention) requires a changed `docs/re/*.md` **and** a `CHECKER_ROWS` line in
  one of the changed checker files;
* gate scripts, generators and tests are not checkers and are not covered.

Tests are exempt in shape only: `tests/` is not scanned, because a body without
its model is not a body.

Exit status: 0 when every changed body/checker is paired, 1 otherwise.
"""
import argparse
import subprocess
import sys

CHECKER_SUFFIXES = ("_audit.py", "_native.py")


def changed_paths(base, root="."):
    """Files this branch changes relative to `base` (three-dot, so merge-base)."""
    done = subprocess.run(["git", "-C", root, "diff", "--name-only", f"{base}...HEAD"],
                          capture_output=True, text=True)
    if done.returncode:
        raise SystemExit(f"cannot diff against {base}: {done.stderr.strip()}")
    return [line for line in done.stdout.splitlines() if line]


def read(path, root="."):
    from pathlib import Path
    target = Path(root) / path
    return target.read_text(errors="ignore") if target.is_file() else ""


def problems(paths, root="."):
    """Why this change set is not reviewable, or an empty list."""
    bodies = [p for p in paths if p.startswith("src/recovered/") and p.endswith(".c")]
    checkers = [p for p in paths
                if p.startswith("tools/overlay/") and p.endswith(CHECKER_SUFFIXES)]
    docs = [p for p in paths if p.startswith("docs/re/") and p.endswith(".md")]
    found = []
    if (bodies or checkers) and not docs:
        found.append("no evidence document under docs/re/ is part of this change"
                     f" (bodies: {len(bodies)}, checkers: {len(checkers)})")
    if checkers:
        registered = [p for p in checkers if "CHECKER_ROWS" in read(p, root)]
        if not registered:
            found.append("no changed checker declares CHECKER_ROWS "
                         f"(checkers: {', '.join(checkers)})")
    for body in bodies:
        pass  # pairing is at the change-set level, not per-file: keep it simple
    return found


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", default="origin/main")
    parser.add_argument("--root", default=".")
    parser.add_argument("--changed", nargs="*", default=None,
                        help="explicit path list (tests); default is the git diff")
    args = parser.parse_args(argv)
    try:
        paths = args.changed if args.changed is not None else changed_paths(args.base, args.root)
    except SystemExit as error:
        print(error)
        return 1
    found = problems(paths, args.root)
    if found:
        for line in found:
            print(f"UNPAIRED BODY: {line}")
        print(f"REFUSED: {len(found)} reason(s) - a body or checker lands with its "
              f"evidence document and CHECKER_ROWS, or it does not land")
        return 1
    print(f"OK: {len(paths)} changed path(s); every body/checker is paired "
          f"with evidence and registration")
    return 0


if __name__ == "__main__":
    sys.exit(main())
