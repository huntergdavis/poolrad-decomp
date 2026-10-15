#!/usr/bin/env python3
"""Report README/docs references to an explicit recovered-test metadata registry."""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import sys
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[2]
SOURCE = re.compile(r"(?<![A-Za-z0-9_])tests/recovered/test_[A-Za-z0-9_.-]+\.c\b")
TARGET = r"poolrad_[A-Za-z0-9_]*_tests"
LINK = re.compile(r"\]\(\s*(<[^>\n]+>|[^\s)]+)(?:\s+[^)]+)?\)")
URL = re.compile(r"https?://[^\s<>)]+")


def canonical_source(value):
    return isinstance(value, str) and bool(re.fullmatch(r"tests/recovered/test_[A-Za-z0-9_.-]+\.c", value))


def names(value):
    if not isinstance(value, list) or any(not isinstance(v, str) or not v for v in value) or len(value) != len(set(value)):
        raise ValueError("registry name lists must contain unique nonempty strings")
    return set(value)


def read_registry(path, root, cmake):
    raw = path.read_bytes()
    registry = json.loads(raw)
    if not isinstance(registry, dict) or type(registry.get("schema_version")) is not int or registry["schema_version"] != 1:
        raise ValueError("registry schema_version must be integer 1")
    if registry.get("kind") != "recovered_test_registration" or registry.get("passed") is not True:
        raise ValueError("registry must be a passing recovered_test_registration report")
    cmake_hash = sha256(cmake.read_bytes()).hexdigest()
    if registry.get("cmake_sha256") != cmake_hash:
        raise ValueError("registry is stale: CMake input hash differs")
    inventory = names(registry.get("inventory"))
    if not inventory or any(not canonical_source(p) for p in inventory):
        raise ValueError("registry inventory must contain canonical flat focused-test C paths")
    current = {p.relative_to(root).as_posix() for p in (root / "tests/recovered").glob("test_*.c") if p.is_file()}
    if current != inventory:
        raise ValueError("registry is stale: focused-test filename inventory differs")
    executables = names(registry.get("all_executable_names"))
    tests = names(registry.get("all_test_names"))
    records = registry.get("registrations")
    if not isinstance(records, list):
        raise ValueError("registry registrations must be a list")
    by_source = {}
    for record in records:
        if not isinstance(record, dict):
            raise ValueError("registry registration must be an object")
        source, target = record.get("source"), record.get("executable")
        test_names = names(record.get("test_names"))
        if not canonical_source(source) or source not in inventory or source in by_source:
            raise ValueError("registry registrations require each inventoried source exactly once")
        if not isinstance(target, str) or target not in executables or len(test_names) != 1 or not test_names <= tests:
            raise ValueError("registry registration references undeclared executable or nonunique test")
        by_source[source] = {"source": source, "executable": target, "test_names": sorted(test_names)}
    if set(by_source) != inventory:
        raise ValueError("registry does not cover its full focused-test inventory")
    expected = {"source_files": len(inventory), "focused_executables": len({r["executable"] for r in by_source.values()}),
                "focused_test_names": len({n for r in by_source.values() for n in r["test_names"]})}
    if any(type(registry.get(key)) is not int or registry[key] != value for key, value in expected.items()):
        raise ValueError("registry counts disagree with registrations")
    issues = registry.get("issues")
    if not isinstance(issues, dict) or any(not isinstance(v, list) or v for v in issues.values()):
        raise ValueError("passing registry must have empty issue lists")
    return by_source, executables | tests, sha256(raw).hexdigest(), cmake_hash


def audit(root, registry_path, cmake, docs=None):
    root = root.resolve()
    records, declared_names, registry_hash, cmake_hash = read_registry(registry_path, root, cmake)
    if docs is None:
        docs = ([root / "README.md"] if (root / "README.md").is_file() else []) + sorted((root / "docs").rglob("*.md"))
    if not docs:
        raise ValueError("no README/documentation Markdown files selected")
    selected = {}
    for doc in docs:
        doc = doc if doc.is_absolute() else root / doc
        resolved = doc.resolve()
        if not resolved.is_relative_to(root) or not resolved.is_file() or resolved.suffix.lower() != ".md":
            raise ValueError(f"documentation must be an existing in-tree Markdown file: {doc}")
        selected[resolved.relative_to(root).as_posix()] = resolved
    target_pattern = re.compile(r"(?<![A-Za-z0-9_])(?:" + "|".join(
        [TARGET] + [re.escape(name) for name in sorted(declared_names, key=len, reverse=True)]) + r")(?![A-Za-z0-9_])")
    references, stale, broken, document_hashes = [], [], [], {}
    for label, doc in sorted(selected.items()):
        raw = doc.read_bytes()
        document_hashes[label] = sha256(raw).hexdigest()
        text = raw.decode("utf-8-sig")
        external = [match.span() for match in URL.finditer(text)]
        for kind, pattern in (("source", SOURCE), ("target", target_pattern)):
            for match in pattern.finditer(text):
                if any(start <= match.start() < end for start, end in external):
                    continue
                value = match.group()
                reference = {"document": label, "line": text.count("\n", 0, match.start()) + 1,
                             "kind": kind, "reference": value}
                references.append(reference)
                if (kind == "source" and value not in records) or (kind == "target" and value not in declared_names):
                    stale.append(reference)
        for match in LINK.finditer(text):
            destination = match.group(1).strip("<>")
            parsed = urlsplit(destination)
            if parsed.scheme or parsed.netloc:
                continue
            path = unquote(parsed.path)
            if not SOURCE.search(path):
                continue
            candidate = root / path.lstrip("/") if path.startswith("/") else doc.parent / path
            candidate = candidate.resolve()
            if not candidate.is_relative_to(root) or candidate.relative_to(root).as_posix() not in records:
                broken.append({"document": label, "line": text.count("\n", 0, match.start()) + 1,
                               "destination": destination})
    covered = {r["reference"] for r in references}
    missing = [record for source, record in sorted(records.items())
               if not ({source, record["executable"], *record["test_names"]} & covered)]
    return {"schema_version": 1, "kind": "recovered_test_documentation", "root": str(root),
            "registry_file": str(registry_path), "registry_sha256": registry_hash,
            "cmake_sha256": cmake_hash, "document_hashes": document_hashes,
            "registered_sources": len(records), "covered_sources": len(records) - len(missing),
            "references": references, "missing_documentation": missing,
            "stale_references": stale, "broken_source_links": broken,
            "passed": not any((missing, stale, broken))}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--registry", type=Path, required=True)
    parser.add_argument("--cmake", type=Path)
    parser.add_argument("--doc", type=Path, action="append")
    parser.add_argument("--json", type=Path)
    args = parser.parse_args(argv)
    try:
        report = audit(args.root, args.registry, args.cmake or args.root / "CMakeLists.txt", args.doc)
        code = 0 if report["passed"] else 1
    except (OSError, ValueError) as error:
        report = {"schema_version": 1, "error": str(error)}
        code = 2
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    if "error" in report:
        print(report["error"], file=sys.stderr)
    else:
        print(f"{report['covered_sources']}/{report['registered_sources']} focused sources documented; "
              f"{len(report['stale_references'])} stale references; "
              f"{len(report['broken_source_links'])} broken source links: {'PASS' if report['passed'] else 'FAIL'}")
        for record in report["missing_documentation"]:
            print(f"missing_documentation: {record['source']}", file=sys.stderr)
        for key in ("stale_references", "broken_source_links"):
            for record in report[key]:
                print(f"{key}: {record}", file=sys.stderr)
    return code


if __name__ == "__main__":
    raise SystemExit(main())
