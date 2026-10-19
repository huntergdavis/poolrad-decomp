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
import os
from pathlib import Path
import re
import subprocess
import tempfile
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


def _cc_commands(tree, root):
    """Every `subprocess.run([...])` whose argv starts with a C compiler."""
    def ev(node):
        if isinstance(node, ast.List):
            return [ev(elt) for elt in node.elts]
        if isinstance(node, ast.Constant) and isinstance(node.value, str):
            return node.value
        if isinstance(node, ast.Name) and node.id == "ROOT":
            return str(root)
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and node.func.id == "str":
            return str(ev(node.args[0]))
        if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Div):
            return str(Path(ev(node.left)) / ev(node.right))
        raise ValueError(ast.dump(node)[:60])
    found = []
    for node in ast.walk(tree):
        if not (isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
                and node.func.attr == "run" and node.args):
            continue
        literal = node.args[0]
        if not isinstance(literal, ast.List):
            continue
        argv = []
        for element in literal.elts:
            try:
                value = ev(element)
            except ValueError:
                # Un-evaluable tail (e.g. str(library) after -o): drop it and
                # stop, keeping the source list that precedes it.
                if argv and argv[-1] == "-o":
                    argv.pop()
                break
            argv.append(value)
        if argv and argv[0] in ("cc", "gcc", "clang"):
            found.append(argv)
    return found


def sweep(root, timeout=120):
    """Best-effort compile of every checker found. Never raises, never gates."""
    results = []
    for folder in ("tests/recovered", "tools/overlay"):
        for path in sorted((root / folder).glob("*.py")):
            text = path.read_text()
            if "src/recovered" not in text:
                continue
            try:
                tree = ast.parse(text, filename=str(path))
            except SyntaxError as error:
                results.append(dict(file=str(path.relative_to(root)), status="PARSE",
                                    detail=str(error)[:120]))
                continue
            commands = _cc_commands(tree, root)
            if not commands:
                continue
            rel = str(path.relative_to(root))
            entry = dict(file=rel, status="NO-COMPILE", detail="")
            argv = list(commands[0])
            argv = [os.environ.get("CC") or "cc" if a == "cc" else a for a in argv]
            out_dir = tempfile.mkdtemp(prefix="sweep-")
            probe = [a for a in argv if a not in ("-o",)] 
            if "-o" in argv:
                cut = argv.index("-o")
                probe = argv[:cut] + ["-o", os.path.join(out_dir, "probe.so")]
            else:
                probe = argv + ["-o", os.path.join(out_dir, "probe.so")]
            try:
                done = subprocess.run(probe, capture_output=True, text=True, timeout=timeout)
            except OSError as error:
                results.append(dict(file=rel, status="NO-CC", detail=str(error)[:120]))
                continue
            except subprocess.TimeoutExpired:
                results.append(dict(file=rel, status="TIMEOUT", detail=str(timeout)))
                continue
            if done.returncode == 0:
                entry = dict(file=rel, status="OK", detail="")
            else:
                first = (done.stderr or "").strip().splitlines()
                detail = (first[0] if first else "cc failed")[:160]
                # A dynamic source list (built at runtime) leaves the argv with
                # no inputs; that is an extraction limit, not a stale checker.
                status = "UNEXTRACTABLE" if "no input files" in detail else "FAIL"
                entry = dict(file=rel, status=status, detail=detail)
            results.append(entry)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rows", nargs="*", help="every row being offered for review")
    parser.add_argument("--all", action="store_true", help="all registered checker rows")
    parser.add_argument("--base-ref", default="origin/main",
                        help="fetched main ref that HEAD must include")
    parser.add_argument("--output", type=Path, help="optional JSON build receipt")
    parser.add_argument("--sweep", action="store_true",
                        help="best-effort compile of every checker; never gates (exit 0)")
    args = parser.parse_args()
    if args.sweep:
        rows = sweep(ROOT)
        bad = [r for r in rows if r["status"] != "OK"]
        for r in rows:
            print(f"{r['status']:>12}  {r['file']}  {r['detail']}")
        print(f"SWEEP: {len(rows)} checkers attempted, {len(bad)} not clean (non-blocking)")
        return 0
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
