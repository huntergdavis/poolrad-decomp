#!/usr/bin/env python3
"""Audit direct literal CTest property declarations for focused recovered tests."""
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


KNOWN_PROPERTIES = {"DISABLED", "TIMEOUT", "WILL_FAIL", "RUN_SERIAL", "LABELS", "WORKING_DIRECTORY"}


def disabled_value(value):
    upper = value.upper()
    if upper in ("1", "ON", "YES", "TRUE", "Y"):
        return True
    if upper in ("0", "OFF", "NO", "FALSE", "N", "IGNORE", "NOTFOUND", "") or upper.endswith("-NOTFOUND"):
        return False
    raise ValueError(f"DISABLED value outside explicit supported boolean constants: {value!r}")


def audit(root, registry_path, cmake, allowed=()):
    root = root.resolve()
    records, _, registry_hash, cmake_hash = read_registry(registry_path, root, cmake)
    focused = {name for record in records.values() for name in record["test_names"]}
    parsed = list(commands(cmake.read_bytes().decode("utf-8-sig").replace("\r\n", "\n")))
    tests = defaultdict(list)
    for command, args, line, scope in parsed:
        if command != "add_test":
            continue
        if len(args) != 4 or args[0] != "NAME" or args[2] != "COMMAND":
            raise ValueError(f"line {line}: expected literal NAME/COMMAND add_test")
        tests[literal(args[1])].append({"line": line, "scope": scope})
    if any(len(locations) != 1 for locations in tests.values()) or not focused <= set(tests):
        raise ValueError("test definitions are duplicate or do not cover focused registry names")
    allowed = {literal(name) for name in allowed}
    if any(not re.fullmatch(r"[A-Z][A-Z0-9_]*", name) for name in allowed):
        raise ValueError("allowed property names must use uppercase letters/digits/underscore")
    assignments, declarations = [], []
    unknown_tests, unknown_properties = [], []
    for command, args, line, scope in parsed:
        if command == "set_tests_properties":
            if "PROPERTIES" not in args:
                raise ValueError(f"line {line}: missing PROPERTIES")
            index = args.index("PROPERTIES")
            test_names, values = args[:index], args[index + 1:]
            if not test_names or not values or len(values) % 2:
                raise ValueError(f"line {line}: expected tests and property/value pairs")
            pairs = list(zip(values[::2], values[1::2]))
        elif command == "set_property" and args and args[0] == "TEST":
            if "PROPERTY" not in args:
                raise ValueError(f"line {line}: missing PROPERTY")
            index = args.index("PROPERTY")
            test_names, values = args[1:index], args[index + 1:]
            if not test_names or len(values) != 2:
                raise ValueError(f"line {line}: expected tests, one property and one literal value")
            pairs = [tuple(values)]
        else:
            continue
        if any(name in ("DIRECTORY", "APPEND", "APPEND_STRING") for name in test_names):
            raise ValueError(f"line {line}: directory/append property forms are unsupported")
        supported_scope(scope)
        declarations.append({"command": command, "line": line, "tests": test_names})
        for name in test_names:
            literal(name)
            if name not in tests:
                unknown_tests.append({"test": name, "line": line})
            elif scope != tests[name][0]["scope"] or line < tests[name][0]["line"]:
                raise ValueError(f"line {line}: property scope/order differs from test definition")
            for property_name, value in pairs:
                literal(property_name)
                if value:
                    literal(value)
                record = {"test": name, "property": property_name, "value": value, "line": line, "command": command}
                assignments.append(record)
                if name in focused and property_name not in KNOWN_PROPERTIES | allowed:
                    unknown_properties.append(record)
                if property_name == "DISABLED":
                    record["disabled"] = disabled_value(value)
    grouped = defaultdict(list)
    for record in assignments:
        if record["test"] in focused:
            grouped[(record["test"], record["property"])].append(record)
    issues = {
        "disabled_focused_tests": sorted({r["test"] for r in assignments if r["test"] in focused and r.get("disabled")}),
        "unknown_tests": unknown_tests, "unknown_focused_properties": unknown_properties,
        "duplicate_focused_properties": [{"test": key[0], "property": key[1], "lines": [r["line"] for r in values]}
                                         for key, values in sorted(grouped.items()) if len(values) > 1],
    }
    return {"schema_version": 1, "kind": "recovered_test_properties", "root": str(root),
            "cmake_sha256": cmake_hash, "registry_sha256": registry_hash,
            "focused_test_count": len(focused), "direct_test_count": len(tests),
            "property_declaration_count": len(declarations), "property_assignment_count": len(assignments),
            "focused_property_assignment_count": sum(r["test"] in focused for r in assignments),
            "known_properties": sorted(KNOWN_PROPERTIES), "explicit_allowed_properties": sorted(allowed),
            "declarations": declarations, "assignments": assignments,
            "issues": issues, "passed": not any(issues.values())}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--registry", type=Path, required=True)
    parser.add_argument("--cmake", type=Path)
    parser.add_argument("--allow-property", action="append", default=[])
    parser.add_argument("--json", type=Path)
    args = parser.parse_args(argv)
    try:
        report = audit(args.root, args.registry, args.cmake or args.root / "CMakeLists.txt", args.allow_property)
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
        print(f"{report['focused_test_count']} focused CTests, {report['property_declaration_count']} direct property declarations: "
              f"{'PASS' if report['passed'] else 'FAIL'}")
        for name, values in report["issues"].items():
            for value in values:
                print(f"{name}: {value}", file=sys.stderr)
    return code


if __name__ == "__main__":
    raise SystemExit(main())
