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

A file that matches the checker pattern but is not a row checker (for example
the queue generator `function_queue_audit.py`, which produces the accounting
pin) is exempt only through an explicit, greppable line in the file itself:

    # NOT_A_ROW_CHECKER: <reason, at least ten characters>

The exemption is recorded rather than hidden, so a rename is never needed. A
marker without a reason exempts nothing. A file that carries the marker and
also declares `CHECKER_ROWS` is refused as a contradiction, and an exempt file
never satisfies the registration requirement for a real checker.

Tests are exempt in shape only: `tests/` is not scanned, because a body without
its model is not a body.

Exit status: 0 when every changed body/checker is paired, 1 otherwise.
"""
import argparse
import re
import subprocess
import sys

CHECKER_SUFFIXES = ("_audit.py", "_native.py")
EXEMPTION = re.compile(r"^# NOT_A_ROW_CHECKER: (?P<reason>\S.{9,})$", re.MULTILINE)


def exemption(text):
    """The stated reason if `text` carries a valid not-a-row-checker marker, else None."""
    match = EXEMPTION.search(text)
    return match.group("reason").strip() if match else None


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


def exempt_generators(paths, root="."):
    """Checker-pattern files that declare themselves not row checkers, with reasons."""
    exempt = {}
    for p in paths:
        if p.startswith("tools/overlay/") and p.endswith(CHECKER_SUFFIXES):
            reason = exemption(read(p, root))
            if reason is not None:
                exempt[p] = reason
    return exempt


def problems(paths, root="."):
    """Why this change set is not reviewable, or an empty list."""
    bodies = [p for p in paths if p.startswith("src/recovered/") and p.endswith(".c")]
    exempt = exempt_generators(paths, root)
    checkers = [p for p in paths
                if p.startswith("tools/overlay/") and p.endswith(CHECKER_SUFFIXES)
                and p not in exempt]
    docs = [p for p in paths if p.startswith("docs/re/") and p.endswith(".md")]
    found = []
    for p in sorted(exempt):
        if "CHECKER_ROWS" in read(p, root):
            found.append(f"{p} declares NOT_A_ROW_CHECKER and CHECKER_ROWS; "
                         "a file is a row checker or it is not")
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
    for path, reason in sorted(exempt_generators(paths, args.root).items()):
        print(f"EXEMPT (not a row checker): {path} - {reason}")
    print(f"OK: {len(paths)} changed path(s); every body/checker is paired "
          f"with evidence and registration")
    return 0


if __name__ == "__main__":
    sys.exit(main())
