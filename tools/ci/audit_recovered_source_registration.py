#!/usr/bin/env python3
"""Audit literal recovered-source registrations without executing CMake or C."""
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
        line = text.count("\n", 0, offset) + 1
        yield name, arguments, line, tuple(contexts)
        if name in starts:
            contexts.append(name)
        elif name in ends:
            if not contexts or contexts.pop() != ends[name]:
                raise ValueError(f"unmatched CMake block end {name}")
    if contexts:
        raise ValueError("unterminated CMake control/definition block")


def recovered_path(value):
    if any(char in value for char in "$;\\*?[]") or value in ("(", ")"):
        raise ValueError(f"unresolved/list/escaped source argument: {value!r}")
    if value.startswith("/") or re.match(r"^[A-Za-z]:", value):
        raise ValueError(f"absolute source argument is outside this audit: {value!r}")
    normalized = posixpath.normpath(value)
    if normalized.startswith("../") or normalized == "..":
        raise ValueError(f"source argument leaves the repository: {value!r}")
    path = Path(normalized)
    if normalized.startswith("src/recovered/") and path.suffix == ".c":
        if path.parent.as_posix() != "src/recovered":
            raise ValueError(f"nested recovered source is outside the flat inventory: {value!r}")
        return normalized
    return None


def read_exclusions(path):
    document = json.loads(path.read_text())
    if not isinstance(document, dict) or set(document) != {"schema_version", "exclusions"}:
        raise ValueError("exclusions require schema_version and exclusions fields")
    if type(document["schema_version"]) is not int or document["schema_version"] != 1 or not isinstance(document["exclusions"], list):
        raise ValueError("unsupported exclusions schema")
    exclusions = {}
    for entry in document["exclusions"]:
        if not isinstance(entry, dict) or set(entry) != {"path", "reason"}:
            raise ValueError("each exclusion requires exactly path and reason")
        value, reason = entry["path"], entry["reason"]
        if not isinstance(value, str) or recovered_path(value) != value:
            raise ValueError(f"exclusion must be a canonical flat recovered C path: {value!r}")
        if not isinstance(reason, str) or not reason.strip():
            raise ValueError(f"exclusion needs a nonempty reason: {value}")
        if value in exclusions:
            raise ValueError(f"duplicate exclusion: {value}")
        exclusions[value] = reason
    return exclusions


def audit_registration(root, cmake_path, exclusions_path, target="poolrad_recovered"):
    root = Path(root).resolve()
    inventory = {path.relative_to(root).as_posix() for path in (root / "src/recovered").glob("*.c") if path.is_file()}
    if not inventory:
        raise ValueError("no src/recovered/*.c source files found")
    cmake_bytes = cmake_path.read_bytes()
    text = cmake_bytes.decode("utf-8-sig").replace("\r\n", "\n")
    registrations = defaultdict(list)
    definitions = 0
    for command, arguments, line, contexts in commands(text):
        if command not in ("add_library", "target_sources") or not arguments or arguments[0] != target:
            continue
        if contexts:
            raise ValueError(f"line {line}: conditional/definition-scoped registration is unresolved: {contexts}")
        source_arguments = arguments[1:]
        if command == "add_library":
            definitions += 1
            if definitions > 1:
                raise ValueError(f"multiple add_library definitions for {target}")
            if any(value in ("ALIAS", "IMPORTED", "INTERFACE") for value in source_arguments):
                raise ValueError(f"line {line}: target must compile its own literal sources")
            source_arguments = [value for value in source_arguments if value not in ("STATIC", "SHARED", "MODULE", "OBJECT", "EXCLUDE_FROM_ALL")]
        else:
            if definitions != 1:
                raise ValueError(f"line {line}: target_sources appears before target definition")
            if not source_arguments or source_arguments[0] not in ("PRIVATE", "PUBLIC") or any(value in ("INTERFACE", "FILE_SET") for value in source_arguments):
                raise ValueError(f"line {line}: only PUBLIC/PRIVATE target_sources are supported")
            source_arguments = [value for value in source_arguments if value not in ("PRIVATE", "PUBLIC")]
        for value in source_arguments:
            source = recovered_path(value)
            if source:
                registrations[source].append({"command": command, "line": line})
    if definitions != 1:
        raise ValueError(f"expected one literal add_library definition for {target}")
    exclusions = read_exclusions(exclusions_path)
    registered = set(registrations)
    missing = sorted(inventory - registered - set(exclusions))
    dangling = sorted(registered - inventory)
    duplicates = sorted(path for path, locations in registrations.items() if len(locations) > 1)
    stale = sorted(set(exclusions) - inventory)
    conflicts = sorted(set(exclusions) & registered)
    passed = not any((missing, dangling, duplicates, stale, conflicts))
    return {"schema_version": 1, "root": str(root), "target": target,
            "cmake_file": str(cmake_path), "cmake_sha256": sha256(cmake_bytes).hexdigest(),
            "exclusions_file": str(exclusions_path), "exclusions_sha256": sha256(exclusions_path.read_bytes()).hexdigest(),
            "source_files": len(inventory), "registered_files": len(registered),
            "registration_entries": sum(map(len, registrations.values())),
            "excluded_files": len(exclusions), "passed": passed,
            "missing_registrations": missing, "dangling_registrations": dangling,
            "duplicate_registrations": duplicates, "stale_exclusions": stale,
            "registered_exclusions": conflicts, "inventory": sorted(inventory),
            "registrations": dict(sorted(registrations.items())), "exclusions": dict(sorted(exclusions.items()))}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--cmake", type=Path, help="CMake file (default: root/CMakeLists.txt)")
    parser.add_argument("--target", default="poolrad_recovered")
    parser.add_argument("--exclusions", type=Path, default=Path(__file__).with_name("recovered_source_exclusions.json"))
    parser.add_argument("--json", type=Path)
    args = parser.parse_args(argv)
    try:
        report = audit_registration(args.root, args.cmake or args.root / "CMakeLists.txt", args.exclusions, args.target)
        code = 0 if report["passed"] else 1
    except (OSError, ValueError) as error:
        report = {"schema_version": 1, "error": str(error)}
        code = 2
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    if "error" in report:
        print(report["error"], file=sys.stderr)
    else:
        print(f"{report['source_files']} sources, {report['registered_files']} registered, "
              f"{report['excluded_files']} excluded: {'PASS' if report['passed'] else 'FAIL'}")
        for name in ("missing_registrations", "dangling_registrations", "duplicate_registrations", "stale_exclusions", "registered_exclusions"):
            for path in report[name]:
                print(f"{name}: {path}", file=sys.stderr)
    return code


if __name__ == "__main__":
    raise SystemExit(main())
