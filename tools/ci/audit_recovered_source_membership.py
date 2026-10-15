#!/usr/bin/env python3
"""Audit hash-bound flat recovered source membership using metadata only."""
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



def member(value):
    if not value or any(char in value for char in "$;\\*?[]") or value in ("(", ")"):
        raise ValueError(f"unresolved source argument: {value!r}")
    normalized = posixpath.normpath(value)
    path = Path(normalized)
    in_scope = not value.startswith("/") and not re.match(r"^[A-Za-z]:", value)
    in_scope = in_scope and path.parent.as_posix() == "src/recovered" and path.suffix == ".c"
    return normalized, bool(in_scope)


def audit(root, cmake, expected):
    if not re.fullmatch(r"[0-9a-fA-F]{64}", expected):
        raise ValueError("expected CMake SHA256 must contain exactly 64 hexadecimal digits")
    raw = cmake.read_bytes()
    actual = sha256(raw).hexdigest()
    if actual != expected.lower():
        raise ValueError(f"stale CMake input: expected {expected.lower()}, found {actual}")
    root = root.resolve()
    inventory = {p.relative_to(root).as_posix() for p in (root / "src/recovered").glob("*.c") if p.is_file()}
    if not inventory:
        raise ValueError("no flat src/recovered/*.c filenames found")
    registrations, outside = defaultdict(list), []
    definitions = 0
    for command, args, line, scope in commands(raw.decode("utf-8-sig").replace("\r\n", "\n")):
        if command not in ("add_library", "target_sources") or not args or args[0] != "poolrad_recovered":
            continue
        if scope:
            raise ValueError(f"line {line}: scoped recovered-source membership is unresolved")
        values = args[1:]
        if command == "add_library":
            definitions += 1
            if definitions != 1 or any(v in ("ALIAS", "IMPORTED", "INTERFACE") for v in values):
                raise ValueError("expected one compiling literal poolrad_recovered library")
            values = [v for v in values if v not in ("STATIC", "SHARED", "MODULE", "OBJECT", "EXCLUDE_FROM_ALL")]
        else:
            if definitions != 1:
                raise ValueError("target_sources precedes recovered library definition")
            if not values or values[0] not in ("PUBLIC", "PRIVATE") or any(v in ("INTERFACE", "FILE_SET") for v in values):
                raise ValueError("only literal PUBLIC/PRIVATE target_sources is supported")
            values = [v for v in values if v not in ("PUBLIC", "PRIVATE")]
        for value in values:
            path, in_scope = member(value)
            location = {"command": command, "line": line, "argument": value}
            if in_scope:
                registrations[path].append(location)
            else:
                outside.append({"path": path, **location})
    if definitions != 1:
        raise ValueError("missing literal poolrad_recovered library definition")
    issues = {
        "missing_members": sorted(inventory - set(registrations)),
        "nonexistent_members": sorted(set(registrations) - inventory),
        "duplicate_members": sorted(p for p, locations in registrations.items() if len(locations) > 1),
        "out_of_scope_members": outside,
    }
    return {"schema_version": 1, "kind": "recovered_source_membership", "root": str(root),
            "cmake_sha256": actual, "expected_cmake_sha256": expected.lower(),
            "source_files": len(inventory), "registered_files": len(registrations),
            "registration_entries": sum(map(len, registrations.values())),
            "inventory": sorted(inventory), "registrations": dict(sorted(registrations.items())),
            "issues": issues, "passed": not any(issues.values())}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--cmake", type=Path)
    parser.add_argument("--expected-cmake-sha256", required=True)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args(argv)
    try:
        report = audit(args.root, args.cmake or args.root / "CMakeLists.txt", args.expected_cmake_sha256)
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
        print(f"{report['source_files']} flat sources, {report['registered_files']} members: "
              f"{'PASS' if report['passed'] else 'FAIL'}")
        for key, values in report["issues"].items():
            for value in values:
                print(f"{key}: {value}", file=sys.stderr)
    return code


if __name__ == "__main__":
    raise SystemExit(main())
