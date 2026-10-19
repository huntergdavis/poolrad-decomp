#!/usr/bin/env python3
"""Compile/load the real row checkers before handoff, without game media.

Registration lives in each checker: CHECKER_ROWS = ("OVL_...._....",).
Its --compile-only mode must use the audit's actual compiler command and
CDLL/symbol binding, then print CHECKER_BUILD_OK. No duplicate source lists.
"""
import argparse
import ast
import csv
import hashlib
import io
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
ROW_ID = re.compile(r"(?:OVL|RES)_[0-9A-F]{4}_[0-9A-F]{4}\Z")
MANIFEST = "docs/re/partial_callee_models.csv"


def discover(root):
    registry = {}
    for folder in ("tests/recovered", "tools/overlay"):
        for path in sorted((root / folder).glob("*.py")):
            source = path.read_text()
            if "CHECKER_ROWS" not in source:
                continue
            for node in ast.parse(source, filename=str(path)).body:
                if not isinstance(node, ast.Assign) or not any(
                        isinstance(t, ast.Name) and t.id == "CHECKER_ROWS"
                        for t in node.targets):
                    continue
                rows = ast.literal_eval(node.value)
                if not isinstance(rows, tuple) or not rows or any(
                        not isinstance(row, str) or not ROW_ID.fullmatch(row)
                        for row in rows) or len(set(rows)) != len(rows):
                    raise ValueError(f"invalid CHECKER_ROWS in {path}")
                for row in rows:
                    registry.setdefault(row, []).append(path.relative_to(root))
    return registry


def select(registry, rows):
    if not rows:
        raise ValueError("no registered rows selected; handoff blocked")
    missing = sorted(set(rows) - registry.keys())
    if missing:
        raise ValueError("unregistered row(s); handoff blocked: " + ", ".join(missing))
    return sorted({path for row in rows for path in registry[row]})


def changed_rows(before, after):
    old = {row["id"]: row for row in csv.DictReader(io.StringIO(before))}
    return {row["id"] for row in csv.DictReader(io.StringIO(after))
            if old.get(row["id"]) != row}


def run_checker(root, path):
    command = [sys.executable, str(root / path), "--compile-only"]
    result = subprocess.run(command, cwd=root, capture_output=True, text=True,
                            timeout=120)
    if result.returncode or "CHECKER_BUILD_OK" not in result.stdout.splitlines():
        raise ValueError(f"checker build failed: {path} (exit {result.returncode})\n"
                         + result.stdout + result.stderr)
    return dict(checker=str(path), sha256=hashlib.sha256(
        (root / path).read_bytes()).hexdigest(), command=command,
        stdout=result.stdout, stderr=result.stderr)


def git(root, *args):
    return subprocess.check_output(["git", "-C", str(root), *args], text=True).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rows", nargs="*", help="every row being offered for review")
    parser.add_argument("--all", action="store_true", help="all registered checker rows")
    parser.add_argument("--base-ref", default="origin/main",
                        help="fetched main ref that HEAD must include")
    parser.add_argument("--output", type=Path, help="optional JSON build receipt")
    args = parser.parse_args()
    if args.all and args.rows:
        parser.error("choose row IDs or --all")
    try:
        base, head = git(ROOT, "rev-parse", args.base_ref), git(ROOT, "rev-parse", "HEAD")
        if subprocess.run(["git", "-C", str(ROOT), "merge-base", "--is-ancestor",
                           base, head], check=False).returncode:
            raise ValueError("HEAD does not include fetched main; update before handoff")
        registry = discover(ROOT)
        added = changed_rows(git(ROOT, "show", f"{base}:{MANIFEST}"),
                             (ROOT / MANIFEST).read_text())
        # CI --all must not silently ignore an undeclared checker for a new
        # manifest row. Existing unrelated rows remain outside this pilot.
        rows = sorted((set(registry) if args.all else set(args.rows)) | added)
        paths = select(registry, rows)
        results = []
        for path in paths:
            print(f"Building {path}", flush=True)
            results.append(run_checker(ROOT, path))
        report = dict(base_ref=args.base_ref, base_commit=base, head_commit=head,
                      rows=rows, checkers=results,
                      scope="compile/load/bind only; no parity or credit claim")
        if args.output:
            args.output.write_text(json.dumps(report, indent=2) + "\n")
        print(f"PASS: {len(paths)} actual checker builds for {len(rows)} rows")
    except (ValueError, SyntaxError, OSError, subprocess.SubprocessError) as error:
        parser.exit(1, f"HANDOFF BLOCKED: {error}\n")


if __name__ == "__main__":
    main()
