#!/usr/bin/env python3
"""Audit literal required library links for registered recovered test targets."""
import argparse
from collections import defaultdict
from hashlib import sha256
import json
from pathlib import Path
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


def supported_scope(scope):
    if scope not in ((), (("if", ("BUILD_TESTING",)),)):
        raise ValueError(f"focused registration outside the literal BUILD_TESTING guard: {scope}")


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


def audit(root, registry_path, cmake, allowed=()):
    root = root.resolve()
    records, _, registry_hash, cmake_hash = read_registry(registry_path, root, cmake)
    focused = {record["executable"] for record in records.values()}
    parsed = list(commands(cmake.read_bytes().decode("utf-8-sig").replace("\r\n", "\n")))
    definitions = defaultdict(list)
    for command, args, line, scope in parsed:
        if command in ("add_executable", "add_library") and args:
            definitions[literal(args[0])].append({"command": command, "arguments": args[1:], "line": line, "scope": scope})
    if "poolrad_recovered" not in definitions or len(definitions["poolrad_recovered"]) != 1:
        raise ValueError("expected exactly one literal poolrad_recovered library definition")
    required = definitions["poolrad_recovered"][0]
    if required["command"] != "add_library" or required["scope"] or any(v in ("ALIAS", "IMPORTED", "INTERFACE") for v in required["arguments"]):
        raise ValueError("poolrad_recovered must be a top-level literal library")
    for target in focused:
        if len(definitions[target]) != 1 or definitions[target][0]["command"] != "add_executable":
            raise ValueError(f"expected exactly one focused executable definition: {target}")
        supported_scope(definitions[target][0]["scope"])
    allowed = {literal(name) for name in allowed}
    required_links, interface_links = defaultdict(list), defaultdict(list)
    unknown_targets, unknown_libraries, declarations = [], [], []
    for command, args, line, scope in parsed:
        if command != "target_link_libraries":
            continue
        if not args:
            raise ValueError(f"line {line}: missing link target")
        target = literal(args[0])
        if target not in definitions:
            unknown_targets.append({"target": target, "line": line})
        if target not in focused:
            continue
        supported_scope(scope)
        if scope != definitions[target][0]["scope"] or line < definitions[target][0]["line"]:
            raise ValueError(f"line {line}: focused link scope/order differs from executable")
        if len(args) < 3 or args[1] not in ("PUBLIC", "PRIVATE", "INTERFACE"):
            raise ValueError(f"line {line}: expected explicit PUBLIC/PRIVATE/INTERFACE library groups")
        visibility = None
        libraries = []
        group_has_library = False
        for value in args[1:]:
            if value in ("PUBLIC", "PRIVATE", "INTERFACE"):
                if visibility is not None and not group_has_library:
                    raise ValueError(f"line {line}: empty link visibility group")
                visibility, group_has_library = value, False
                continue
            literal(value)
            if value in ("debug", "optimized", "general") or value.startswith("-") or "/" in value:
                raise ValueError(f"line {line}: conditional, option or path library is unsupported")
            group_has_library = True
            location = {"target": target, "library": value, "visibility": visibility, "line": line}
            libraries.append(location)
            if value not in definitions and value not in allowed:
                unknown_libraries.append(location)
            if value == "poolrad_recovered":
                (interface_links if visibility == "INTERFACE" else required_links)[target].append(location)
        if not group_has_library:
            raise ValueError(f"line {line}: empty link visibility group")
        declarations.append({"target": target, "line": line, "libraries": libraries})
    issues = {
        "missing_required_links": sorted(t for t in focused if not required_links[t]),
        "duplicate_required_links": sorted(t for t in focused if len(required_links[t]) > 1),
        "interface_required_links": sorted(interface_links),
        "unknown_link_targets": unknown_targets, "unknown_libraries": unknown_libraries,
    }
    return {"schema_version": 1, "kind": "recovered_test_links", "root": str(root),
            "cmake_sha256": cmake_hash, "registry_sha256": registry_hash,
            "focused_targets": sorted(focused), "focused_target_count": len(focused),
            "required_link_count": sum(map(len, required_links.values())),
            "allowed_external_libraries": sorted(allowed), "declarations": declarations,
            "issues": issues, "passed": not any(issues.values())}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--registry", type=Path, required=True)
    parser.add_argument("--cmake", type=Path)
    parser.add_argument("--allow-library", action="append", default=[])
    parser.add_argument("--json", type=Path)
    args = parser.parse_args(argv)
    try:
        report = audit(args.root, args.registry, args.cmake or args.root / "CMakeLists.txt", args.allow_library)
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
        print(f"{report['focused_target_count']} focused targets, {report['required_link_count']} required links: "
              f"{'PASS' if report['passed'] else 'FAIL'}")
        for name, values in report["issues"].items():
            for value in values:
                print(f"{name}: {value}", file=sys.stderr)
    return code


if __name__ == "__main__":
    raise SystemExit(main())
