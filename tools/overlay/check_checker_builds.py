#!/usr/bin/env python3
"""Compile/load the real row checkers before handoff, without game media.

Registration lives in each checker: CHECKER_ROWS = ("OVL_...._....",).
Its --compile-only mode must use the audit's actual compiler command and
CDLL/symbol binding, then print CHECKER_BUILD_OK. No duplicate source lists.
"""
import argparse
import importlib.util
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


def _cc_commands(tree, root, strict=False):
    """Every `subprocess.run([...])` whose argv starts with a C compiler.

    Source lists are frequently assembled into a module- or function-level
    name and then splatted; resolve those so the real paths come back.
    """
    constants = {}
    for node in ast.walk(tree):
        if isinstance(node, ast.Assign) and len(node.targets) == 1:
            target, value = node.targets[0], node.value
            if isinstance(target, ast.Name):
                constants[target.id] = value

    def ev(node, depth=0, env=None):
        env = env or {}
        if depth > 32:
            raise ValueError("deep")
        if isinstance(node, ast.Constant) and isinstance(node.value, str):
            return node.value
        if isinstance(node, (ast.List, ast.Tuple)):
            out = []
            for item in node.elts:
                if isinstance(item, ast.Starred):
                    out.extend(ev(item.value, depth + 1, env))
                else:
                    out.append(ev(item, depth + 1, env))
            return out
        if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Add):
            return ev(node.left, depth + 1, env) + ev(node.right, depth + 1, env)
        if isinstance(node, ast.JoinedStr):
            return "".join(ev(part.value, depth + 1, env) if isinstance(part, ast.FormattedValue)
                           else part.value for part in node.values)
        if (isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
                and node.func.attr == "split" and not node.args):
            return ev(node.func.value, depth + 1, env).split()
        if isinstance(node, ast.Name):
            if node.id in env:
                return env[node.id]
            if node.id == "ROOT":
                return str(root)
            if node.id in constants:
                return ev(constants[node.id], depth + 1, env)
            raise ValueError(node.id)
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and node.func.id == "str":
            return str(ev(node.args[0], depth + 1, env))
        if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Div):
            return str(Path(ev(node.left, depth + 1, env)) / ev(node.right, depth + 1, env))
        if isinstance(node, (ast.ListComp, ast.GeneratorExp)) and len(node.generators) == 1:
            # `[str(ROOT/'src/recovered'/s) for s in SOURCES]`
            generator = node.generators[0]
            if generator.ifs or not isinstance(generator.target, ast.Name):
                raise ValueError("comp")
            items = ev(generator.iter, depth + 1, env)
            if not isinstance(items, list):
                raise ValueError("comp")
            return [ev(node.elt, depth + 1, dict(env, **{generator.target.id: item}))
                    for item in items]
        raise ValueError(ast.dump(node)[:40])

    def argv_of(node):
        """Flatten a list expression, expanding `*name` splats in place."""
        def elements(expr, depth=0):
            if not isinstance(expr, ast.AST):
                raise ValueError("unresolved compiler command")
            if depth > 32:
                raise ValueError("deep compiler argument expression")
            if isinstance(expr, (ast.List, ast.Tuple)):
                return expr.elts
            if isinstance(expr, ast.BinOp) and isinstance(expr.op, ast.Add):
                return elements(expr.left, depth + 1) + elements(expr.right, depth + 1)
            if isinstance(expr, ast.Name) and expr.id in constants:
                return elements(constants[expr.id], depth + 1)
            values = ev(expr)
            if not isinstance(values, list):
                raise ValueError("not a compiler argument list")
            return [ast.Constant(value=v) for v in values]
        try:
            items = elements(node)
        except ValueError:
            if strict:
                raise ValueError("opaque concatenated compiler inputs")
            return None
        out = None
        for element in items:
            if isinstance(element, ast.Starred):
                try:
                    value = ev(element.value)
                except ValueError:
                    if strict:
                        raise ValueError("opaque expanded compiler inputs")
                    return None
                if not isinstance(value, list):
                    return None
                out = (out or []) + value
                continue
            try:
                out = (out or []) + [ev(element)]
            except ValueError:
                if strict:
                    if out and out[-1] in ("-o", "-I", "-L"):
                        out.append("<dynamic-option>")
                        continue
                    raise ValueError("opaque compiler input")
                # Un-evaluable tail (typically the -o destination): stop here
                # and let the caller substitute its own probe output.
                return (out or []) + [None]
        return out

    found = []
    for node in ast.walk(tree):
        if not (isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
                and node.func.attr == "run" and node.args):
            continue
        if strict:
            candidate = node.args[0]
            if isinstance(candidate, ast.Name):
                candidate = constants.get(candidate.id)
            while isinstance(candidate, ast.BinOp) and isinstance(candidate.op, ast.Add):
                candidate = candidate.left
            if not isinstance(candidate, (ast.List, ast.Tuple)) or not candidate.elts:
                continue
            try:
                compiler = ev(candidate.elts[0])
            except ValueError:
                continue
            if compiler not in ("cc", "gcc", "clang"):
                continue
        argv = argv_of(node.args[0])
        if argv is None and isinstance(node.args[0], ast.Name):
            try:
                argv = argv_of(constants.get(node.args[0].id))
            except (ValueError, KeyError):
                argv = None
        if not (argv and isinstance(argv[0], str) and argv[0] in ("cc", "gcc", "clang")):
            continue
        # Extraction stops at the first un-evaluable element, so the tail may
        # be a dangling flag or a placeholder. Trim it: a leftover `-I` would
        # otherwise swallow the output flag we append.
        while argv and (argv[-1] is None or argv[-1] in ("-I", "-D", "-L", "-o", "-include")):
            argv.pop()
        if argv:
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
                # A cc call we could not reconstruct, or none at all.
                if 'subprocess.run' in text:
                    results.append(dict(file=str(path.relative_to(root)),
                                        status="UNEXTRACTABLE",
                                        detail="cc argv not statically resolvable"))
                continue
            rel = str(path.relative_to(root))
            entry = dict(file=rel, status="NO-COMPILE", detail="")
            if not any(str(a).endswith(".c") for a in commands[0]):
                results.append(dict(file=rel, status="UNEXTRACTABLE",
                                    detail="no source paths extracted"))
                continue
            source = list(commands[0])
            output = os.path.join(tempfile.mkdtemp(prefix="sweep-"), "probe.so")
            # Substitute -o's destination IN PLACE and drop any un-evaluable
            # placeholder; never reorder or add a second -o.
            probe, index, seen_output = [], 0, False
            while index < len(source):
                item = source[index]
                if item == "cc":
                    item = os.environ.get("CC") or "cc"
                if item is None:
                    index += 1
                    continue
                probe.append(item)
                if item == "-o" and index + 1 < len(source) and not seen_output:
                    probe.append(output)
                    seen_output = True
                    index += 2
                    continue
                index += 1
            if not seen_output:
                probe += ["-o", output]
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


PR_EVENTS = ("pull_request", "pull_request_target", "merge_group")
# The hygiene workflow triggers on `push` as well as `pull_request`, so a branch
# push arrives as GITHUB_EVENT_NAME=push with main legitimately ahead of HEAD.
# Any CI run therefore uses the merge-base relation check; only a local run (no
# CI environment at all) requires HEAD to include the fetched base.
CI_ENV = ("GITHUB_ACTIONS", "GITHUB_EVENT_NAME", "CI")


PINNED_DEPENDENCIES = (("capstone", "5.0.7"), ("unicorn", "2.1.4"))
ENVIRONMENT_EXIT = 3


def missing_dependencies():
    """Which pinned checker dependencies this interpreter cannot import.

    A missing import is an ENVIRONMENT failure, not a tree failure. The two must
    look different, because the failure mode to engineer out is a human reading
    "ModuleNotFoundError: capstone" as "my change broke something" and going
    hunting through the diff. This returns the missing names so main() can say so
    in one unmissable line, and it uses find_spec so detection itself cannot
    raise.
    """
    missing = []
    for name, version in PINNED_DEPENDENCIES:
        try:
            found = importlib.util.find_spec(name) is not None
        except (ImportError, ValueError):
            found = False
        if not found:
            missing.append(f"{name}=={version}")
    return missing


def environment_message(missing):
    return ("ENVIRONMENT MISSING: " + ", ".join(missing) +
            " not importable - this is NOT a tree failure. Create a venv and "
            "pin the checker dependencies (python3 -m venv .venv && "
            "pip install capstone==5.0.7 unicorn==2.1.4), then re-run. See the "
            "local setup note in docs/re/REVIEW_LANE_HANDOFF.md.")


def _run_git(root, *args):
    return subprocess.run(["git", "-C", str(root), *args],
                          capture_output=True, text=True)


def precondition_error(root, base_ref="origin/main", *, require_current_main=False,
                       event=None, ci=None):
    """Why this tree may not be handed off, or None if it may.

    A local pre-handoff run must be current: HEAD has to include fetched main.
    In pull-request CI that cannot hold - the merge ref is built from the PR
    head while main keeps moving - so there the requirement relaxes to "the two
    histories are related", which still fails closed on a shallow or orphan
    checkout. The checker compilation itself is never relaxed.
    """
    base = _run_git(root, "rev-parse", base_ref)
    head = _run_git(root, "rev-parse", "HEAD")
    if base.returncode or head.returncode:
        return f"cannot resolve {base_ref} or HEAD; fetch before handoff"
    base, head = base.stdout.strip(), head.stdout.strip()
    if ci is None:
        ci = any(os.environ.get(name) for name in CI_ENV)
    relaxed = ci or event in PR_EVENTS
    strict = require_current_main or not relaxed
    if strict:
        if _run_git(root, "merge-base", "--is-ancestor", base, head).returncode:
            return "HEAD does not include fetched main; update before handoff"
        return None
    related = _run_git(root, "merge-base", base, head)
    if related.returncode or not related.stdout.strip():
        return "no merge base between HEAD and fetched main; cannot verify the tree"
    if _run_git(root, "merge-base", "--is-ancestor", related.stdout.strip(),
                base).returncode:
        return "merge base is not an ancestor of fetched main"
    return None


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rows", nargs="*", help="every row being offered for review")
    parser.add_argument("--all", action="store_true", help="all registered checker rows")
    parser.add_argument("--base-ref", default="origin/main",
                        help="fetched main ref that HEAD must include")
    parser.add_argument("--output", type=Path, help="optional JSON build receipt")
    parser.add_argument("--require-current-main", action="store_true",
                        help="require HEAD to include the base ref (local pre-handoff "
                             "runs); pull-request CI checks the merge-base relation "
                             "instead, because main moves under the PR head")
    parser.add_argument("--sweep", action="store_true",
                        help="best-effort compile of every checker; never gates (exit 0)")
    parser.add_argument("--execute", action="store_true",
                        help="run the selected real differentials on a media host (0 agree / 1 fail / 4 cannot run)")
    parser.add_argument("--all-touching", type=Path,
                        help="execute every registered checker selected by a shared recovered C source")
    parser.add_argument("--selection-only", action="store_true",
                        help="print derived execution selection without claiming agreement")
    args = parser.parse_args(argv)
    if args.execute or args.all_touching or args.selection_only:
        import checker_execution as execution
        try:
            if args.sweep or (args.all and args.rows):
                raise ValueError("choose one execution selection; --sweep is build-only")
            missing = missing_dependencies()
            if missing:
                raise ValueError(environment_message(missing))
            problem = precondition_error(ROOT, args.base_ref, require_current_main=True)
            if problem:
                raise ValueError(problem)
            registry = discover(ROOT)
            source_receipt = {}
            if args.all_touching:
                if args.rows or args.all:
                    raise ValueError("--all-touching cannot be combined with rows or --all")
                source = args.all_touching
                if source.is_absolute():
                    source = source.resolve().relative_to(ROOT.resolve())
                else:
                    source = Path(os.path.normpath(source))
                if source not in execution.cmake_sources(ROOT) or not (ROOT / source).is_file():
                    raise ValueError("source is not a current recovered CMake source: " + str(source))
                source_receipt = execution.source_map(ROOT, sorted({p for ps in registry.values() for p in ps}), _cc_commands)
                paths = sorted(p for p, receipt in source_receipt.items() if str(source) in receipt['sources'])
                if not paths:
                    raise ValueError("no registered checker maps to " + str(source))
            else:
                paths = select(registry, sorted(registry) if args.all else args.rows)
            profiles = execution.load_profiles(ROOT)
            absent = [str(p) for p in paths if str(p) not in profiles]
            if absent:
                raise ValueError("no differential execution profile: " + ', '.join(absent))
            if args.selection_only:
                print(json.dumps(dict(scope="selection only; no execution or agreement", checkers=[
                    dict(checker=str(p), rows=sorted(r for r, ps in registry.items() if p in ps),
                         args=profiles[str(p)]['args'], **source_receipt.get(p, {})) for p in paths]), indent=2))
                return 0
            metadata = dict(base_commit=git(ROOT, "rev-parse", args.base_ref), head_commit=git(ROOT, "rev-parse", "HEAD"),
                            selection={str(p): source_receipt[p] for p in paths if p in source_receipt},
                            gate_sha256={p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in
                                         ("tools/overlay/check_checker_builds.py", "tools/overlay/checker_execution.py", execution.PROFILES)})
            status, report = execution.execute(ROOT, registry, paths, profiles, args.output, metadata=metadata)
            if args.output:
                args.output.write_text(json.dumps(report, indent=2) + "\n")
            return status
        except (ValueError, OSError, subprocess.CalledProcessError) as exc:
            print("CANNOT_RUN: " + str(exc))
            return execution.CANNOT_RUN
    missing = missing_dependencies()
    if missing and not getattr(args, "compile_only", False):
        print(environment_message(missing))
        return ENVIRONMENT_EXIT
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
        problem = precondition_error(ROOT, args.base_ref,
                                     require_current_main=args.require_current_main,
                                     event=os.environ.get("GITHUB_EVENT_NAME"),
                                     ci=any(os.environ.get(name) for name in CI_ENV))
        if problem:
            raise ValueError(problem)
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
    sys.exit(main())
