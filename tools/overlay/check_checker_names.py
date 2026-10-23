#!/usr/bin/env python3
"""Undefined-name gate for every registered row checker (no media needed).

check_checker_builds.py --compile-only compiles and binds a checker without EXECUTING its hooks, so a hook that reads a name the
module never defines (#1437: an unimported Unicorn constant) passes CI and fails only when a lane runs the audit with media. This
gate resolves, with the standard library's symtable, every name that a function or the module body reads as a GLOBAL against
the module's own bindings (assignments, defs, classes, imports, for/with/except targets, and names a function assigns under
`global`) and the builtins. An unresolved name is a
NameError waiting for that line to run.

A module with `from x import *` is resolved through x's own bindings when x is a sibling file; names injected some other way
(globals()[...], sys.modules tricks) are not modelled: such a module must define the name normally to pass.

    python3 tools/overlay/check_checker_names.py            # every registered checker
    python3 tools/overlay/check_checker_names.py FILE ...   # just these files
"""
import argparse
import builtins
import symtable
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
ROOT = Path(__file__).resolve().parents[2]
BUILTINS = set(dir(builtins)) | {"__file__", "__name__", "__doc__", "__builtins__", "__spec__", "__loader__", "__package__",
                                 "__path__", "__annotations__", "__dict__", "__qualname__", "__module__", "__class__"}


def module_bindings(table, path, seen=None):
    """Names the module scope binds, following `from sibling import *`."""
    names = {s.get_name() for s in table.get_symbols() if s.is_assigned() or s.is_imported() or s.is_namespace()}

    def declared(scope):                                  # `global X` then an assignment of X inside a function also binds X
        for s in scope.get_symbols():
            if s.is_declared_global() and (s.is_assigned() or s.is_imported()):
                names.add(s.get_name())
        for child in scope.get_children():
            declared(child)
    declared(table)
    seen = seen or set()
    names.discard("*")
    for line in path.read_text().splitlines():            # star imports, from the source text (symtable does not list '*' on every Python)
        line = line.strip()
        if line.startswith("from ") and line.endswith("import *"):
            sibling = path.parent / (line.split()[1].replace(".", "/") + ".py")
            if sibling.exists() and sibling not in seen:
                seen.add(sibling)
                names |= module_bindings(symtable.symtable(sibling.read_text(), str(sibling), "exec"), sibling, seen)
    return names


def undefined(path):
    """[(line-less) scope, name] for every global read the module does not bind."""
    source = Path(path).read_text()
    table = symtable.symtable(source, str(path), "exec")
    bound = module_bindings(table, Path(path)) | BUILTINS
    out = []

    def walk(scope, where):
        for symbol in scope.get_symbols():
            name = symbol.get_name()
            if not symbol.is_referenced():
                continue
            is_module = scope.get_type() == "module"
            global_read = (is_module and not symbol.is_assigned() and not symbol.is_imported()) or \
                          (not is_module and (symbol.is_global() or symbol.is_declared_global()) and not symbol.is_local())
            if global_read and name not in bound:
                out.append((where, name))
        for child in scope.get_children():
            walk(child, f"{where}.{child.get_name()}" if where else child.get_name())

    walk(table, "")
    return sorted(set(out))


def shown(path):
    path = Path(path)
    return path.relative_to(ROOT) if path.is_absolute() and path.is_relative_to(ROOT) else path


def registered_files(root=ROOT):
    from check_checker_builds import discover
    return sorted({p for paths in discover(root).values() for p in paths})


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("files", nargs="*", type=Path)
    args = parser.parse_args(argv)
    files = args.files or [ROOT / p for p in registered_files()]
    bad = 0
    for path in files:
        for where, name in undefined(path):
            print(f"UNDEFINED NAME: {shown(path)}: {name!r} read in {where or '<module>'}")
            bad += 1
    if bad:
        print(f"REFUSED: {bad} undefined name(s) in {len(files)} checker file(s); each is a NameError when that line runs")
        return 1
    print(f"OK: {len(files)} checker file(s); every global name a checker reads is bound or builtin")
    return 0


if __name__ == "__main__":
    sys.exit(main())
