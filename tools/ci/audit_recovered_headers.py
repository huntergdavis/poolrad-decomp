#!/usr/bin/env python3
"""Compile every recovered public header independently as strict C11."""
import argparse
from concurrent.futures import ThreadPoolExecutor
from hashlib import sha256
import json
import math
import os
from pathlib import Path
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
FLAGS = ["-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
         "-fsyntax-only", "-x", "c"]


def diagnostic(value):
    if isinstance(value, bytes):
        return value.decode("utf-8", errors="replace")
    return value or ""


def check_header(header, include_root, command, timeout):
    name = header.relative_to(include_root).as_posix()
    result = {"header": name, "sha256": sha256(header.read_bytes()).hexdigest()}
    source = f"#include <{name}>\nint main(void) {{ return 0; }}\n"
    try:
        completed = subprocess.run(command, input=source, capture_output=True,
                                   text=True, timeout=timeout, check=False)
        result.update(status="passed" if completed.returncode == 0 else "failed",
                      returncode=completed.returncode, stdout=completed.stdout,
                      stderr=completed.stderr)
    except subprocess.TimeoutExpired as error:
        result.update(status="timeout", returncode=None,
                      stdout=diagnostic(error.stdout), stderr=diagnostic(error.stderr))
    except OSError as error:
        result.update(status="compiler_error", returncode=None, stdout="", stderr=str(error))
    return result


def audit_headers(include_root, compiler, jobs=4, timeout=10.0):
    include_root = Path(include_root).resolve()
    headers = sorted((include_root / "poolrad/recovered").glob("*.h"))
    if not headers:
        raise ValueError(f"no public headers in {include_root / 'poolrad/recovered'}")
    if not compiler:
        raise ValueError("compiler command is empty")
    version = subprocess.run([*compiler, "--version"], capture_output=True,
                             text=True, timeout=timeout, check=False)
    if version.returncode != 0:
        raise ValueError(f"compiler --version failed: {version.stderr.strip()}")
    version_lines = (version.stdout or version.stderr).splitlines()
    command = [*compiler, *FLAGS, "-I", str(include_root), "-"]
    with ThreadPoolExecutor(max_workers=jobs) as executor:
        results = list(executor.map(
            lambda header: check_header(header, include_root, command, timeout), headers))
    passed = sum(result["status"] == "passed" for result in results)
    return {"schema_version": 1, "include_root": str(include_root),
            "compiler_argv": compiler, "compiler_version": version_lines[0] if version_lines else "unreported",
            "compile_argv": command, "timeout_seconds": timeout,
            "headers_discovered": len(headers), "headers_checked": len(results),
            "passed": passed, "failed": len(results) - passed,
            "timed_out": sum(result["status"] == "timeout" for result in results),
            "results": results}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--include-root", type=Path, default=ROOT / "include")
    parser.add_argument("--cc", default=os.environ.get("CC", "cc"),
                        help="compiler command, optionally with wrapper arguments; never shell-evaluated")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--timeout", type=float, default=10.0,
                        help="seconds per compiler invocation")
    parser.add_argument("--json", type=Path, help="write complete ordered diagnostics and header hashes")
    args = parser.parse_args(argv)
    if args.jobs < 1 or not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("jobs and timeout must be positive")
    try:
        report = audit_headers(args.include_root, shlex.split(args.cc), args.jobs, args.timeout)
        exit_code = 2 if any(result["status"] == "compiler_error" for result in report["results"]) else int(report["failed"] > 0)
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        report = {"schema_version": 1, "error": str(error), "headers_checked": 0}
        exit_code = 2
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    if "error" in report:
        print(report["error"], file=sys.stderr)
    else:
        print(f"Checked {report['headers_checked']} headers: {report['passed']} passed, "
              f"{report['failed']} failed ({report['timed_out']} timed out)")
        for result in report["results"]:
            if result["status"] != "passed":
                print(f"{result['header']}: {result['status']}", file=sys.stderr)
                print("\n".join(result["stderr"].splitlines()[:6]), file=sys.stderr)
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
