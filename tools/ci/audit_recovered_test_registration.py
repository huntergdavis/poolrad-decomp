#!/usr/bin/env python3
"""Audit direct recovered test registrations using filenames and CMake text only."""
import argparse
from collections import defaultdict
from hashlib import sha256
import json
from pathlib import Path
import posixpath
import re
import sys

ROOT = Path(__file__).resolve().parents[2]

def bracket(text, start):
    match = re.match(r"\[(=*)\[", text[start:])
    if not match:
        return None
    closing = "]" + match.group(1) + "]"
    end = text.find(closing, start + len(match.group(0)))
    if end < 0:
        raise ValueError("unterminated CMake bracket argument/comment")
    content = text[start + len(match.group(0)):end]
    if content.startswith("\n"):
        content = content[1:]
    return content, end + len(closing)


def tokens(text):
    index = 0
    while index < len(text):
        if text[index].isspace():
            index += 1
            continue
        if text[index] == "#":
            block = bracket(text, index + 1)
            if block:
                index = block[1]
            else:
                end = text.find("\n", index)
                index = len(text) if end < 0 else end + 1
            continue
        start = index
        if text[index] in "()":
            yield text[index], text[index], start
            index += 1
            continue
        block = bracket(text, index)
        if block:
            yield "word", block[0], start
            index = block[1]
            continue
        quoted = text[index] == '"'
        if quoted:
            index += 1
        value = []
        while index < len(text):
            char = text[index]
            if quoted and char == '"':
                index += 1
                break
            if not quoted and (char.isspace() or char in "()#"):
                break
            if char == "\\":
                index += 1
                if index >= len(text):
                    raise ValueError("unterminated CMake escape")
                char = text[index]
                if char == "\n":
                    index += 1
                    continue
                if char in "nrt":
                    char = {"n": "\n", "r": "\r", "t": "\t"}[char]
                elif re.fullmatch(r"[A-Za-z0-9]", char):
                    raise ValueError(f"unsupported CMake escape: {char!r}")
            value.append(char)
            index += 1
        else:
            if quoted:
                raise ValueError("unterminated CMake quoted argument")
        yield "word", "".join(value), start


def commands(text):
    stream = list(tokens(text))
    index, contexts = 0, []
    starts = {"if", "foreach", "while", "function", "macro"}
    ends = {"end" + name: name for name in starts}
    while index < len(stream):
        kind, name, offset = stream[index]
        if kind != "word" or not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", name):
            raise ValueError("expected a CMake command name")
        index += 1
        if index >= len(stream) or stream[index][0] != "(":
            raise ValueError(f"expected '(' after CMake command {name}")
        index += 1
        depth, arguments = 1, []
        while index < len(stream) and depth:
            kind, value, _ = stream[index]
            index += 1
            if kind == "(":
                depth += 1
            elif kind == ")":
                depth -= 1
            if depth:
                arguments.append(value)
        if depth:
            raise ValueError(f"unterminated CMake command {name}")
        name = name.lower()
        if name in ("else", "elseif"):
            raise ValueError("alternate CMake branches are outside this literal audit")
        line = text.count("\n", 0, offset) + 1
        yield name, arguments, line, tuple(contexts)
        if name in starts:
            contexts.append((name, tuple(arguments)))
        elif name in ends:
            if not contexts or contexts.pop()[0] != ends[name]:
                raise ValueError(f"unmatched CMake block end {name}")
    if contexts:
        raise ValueError("unterminated CMake control/definition block")



def literal(value):
    if not value or any(char in value for char in "$;\\*?[]") or value in ("(", ")"):
        raise ValueError(f"unresolved literal argument: {value!r}")
    return value


def source_path(value):
    literal(value)
    if value.startswith("/") or re.match(r"^[A-Za-z]:", value):
        raise ValueError(f"absolute source path is unsupported: {value!r}")
    value = posixpath.normpath(value)
    if value == ".." or value.startswith("../"):
        raise ValueError(f"source path leaves repository: {value!r}")
    return value


def focused_source(value):
    path = Path(value)
    return path.parent.as_posix() == "tests/recovered" and path.name.startswith("test_") and path.suffix == ".c"


def supported_scope(scope):
    if scope not in ((), (("if", ("BUILD_TESTING",)),)):
        raise ValueError(f"focused registration outside the literal BUILD_TESTING guard: {scope}")


def audit(root, cmake_path):
    root = Path(root).resolve()
    inventory = {p.relative_to(root).as_posix() for p in (root / "tests/recovered").glob("test_*.c") if p.is_file()}
    if not inventory:
        raise ValueError("no tests/recovered/test_*.c filenames found")
    raw = cmake_path.read_bytes()
    parsed = list(commands(raw.decode("utf-8-sig").replace("\r\n", "\n")))
    executables, source_locations, scopes = defaultdict(list), defaultdict(list), {}
    ctests = defaultdict(list)
    for command, args, line, scope in parsed:
        if command == "add_executable":
            if not args:
                raise ValueError(f"line {line}: missing executable name")
            name = literal(args[0])
            values = args[1:]
            if any(v in ("ALIAS", "IMPORTED") for v in values):
                raise ValueError(f"line {line}: ALIAS/IMPORTED executable unsupported")
            values = [v for v in values if v not in ("WIN32", "MACOSX_BUNDLE", "EXCLUDE_FROM_ALL")]
            paths = [source_path(v) for v in values]
            executables[name].append({"line": line, "sources": paths})
            scopes[name] = scope
            for path in paths:
                source_locations[path].append({"target": name, "line": line, "scope": scope})
        elif command == "add_test":
            if len(args) != 4 or args[0] != "NAME" or args[2] != "COMMAND":
                raise ValueError(f"line {line}: expected add_test(NAME literal COMMAND executable)")
            name, target = literal(args[1]), literal(args[3])
            ctests[name].append({"target": target, "line": line, "scope": scope})
    for command, args, line, scope in parsed:
        if command != "target_sources" or not args or args[0] not in executables:
            continue
        target = args[0]
        if not args[1:] or args[1] not in ("PUBLIC", "PRIVATE") or any(v in ("INTERFACE", "FILE_SET") for v in args[1:]):
            raise ValueError(f"line {line}: unsupported executable target_sources")
        if line < executables[target][0]["line"]:
            raise ValueError(f"line {line}: target_sources precedes executable")
        for value in args[1:]:
            if value in ("PUBLIC", "PRIVATE"):
                continue
            path = source_path(value)
            source_locations[path].append({"target": target, "line": line, "scope": scope})
    focused = {path: locations for path, locations in source_locations.items() if focused_source(path)}
    targets = {location["target"] for locations in focused.values() for location in locations}
    for locations in focused.values():
        for location in locations:
            supported_scope(location["scope"])
            supported_scope(scopes[location["target"]])
            if location["scope"] != scopes[location["target"]]:
                raise ValueError("executable and focused source have different guards")
    target_tests = defaultdict(list)
    for name, locations in ctests.items():
        for location in locations:
            target_tests[location["target"]].append(name)
            if location["target"] in targets:
                supported_scope(location["scope"])
                if location["scope"] != scopes[location["target"]]:
                    raise ValueError("focused executable and add_test have different guards")
    missing = sorted(inventory - set(focused))
    dangling = sorted(set(focused) - inventory)
    duplicates = sorted(path for path, locations in focused.items() if len(locations) > 1)
    duplicate_targets = sorted(name for name, locations in executables.items() if len(locations) > 1)
    duplicate_names = sorted(name for name, locations in ctests.items() if len(locations) > 1)
    unknown_commands = sorted({v["target"] for locations in ctests.values() for v in locations if v["target"] not in executables})
    missing_tests = sorted(target for target in targets if not target_tests[target])
    duplicate_target_tests = sorted(target for target in targets if len(target_tests[target]) > 1)
    issues = {"missing_sources": missing, "dangling_sources": dangling,
              "duplicate_sources": duplicates, "duplicate_executables": duplicate_targets,
              "duplicate_test_names": duplicate_names, "unknown_test_commands": unknown_commands,
              "untested_executables": missing_tests, "multiple_tests_per_executable": duplicate_target_tests}
    records = [{"source": path, "executable": loc["target"],
                "test_names": sorted(target_tests[loc["target"]]), "source_line": loc["line"],
                "executable_line": executables[loc["target"]][0]["line"]}
               for path, locations in sorted(focused.items()) for loc in locations]
    return {"schema_version": 1, "kind": "recovered_test_registration", "root": str(root),
            "cmake_file": str(cmake_path), "cmake_sha256": sha256(raw).hexdigest(),
            "inventory": sorted(inventory), "source_files": len(inventory),
            "focused_executables": len(targets), "focused_test_names": len({name for t in targets for name in target_tests[t]}),
            "all_test_names": sorted(ctests), "all_executable_names": sorted(executables),
            "registrations": records, "issues": issues, "passed": not any(issues.values())}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--cmake", type=Path)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args(argv)
    try:
        report = audit(args.root, args.cmake or args.root / "CMakeLists.txt")
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
        print(f"{report['source_files']} focused sources, {report['focused_executables']} executables, "
              f"{report['focused_test_names']} CTest names: {'PASS' if report['passed'] else 'FAIL'}")
        for name, values in report["issues"].items():
            for value in values:
                print(f"{name}: {value}", file=sys.stderr)
    return code


if __name__ == "__main__":
    raise SystemExit(main())
