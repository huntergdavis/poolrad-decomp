#!/usr/bin/env python3
"""Label every strict credit with the kind of evidence a reader can actually re-run (VP i20849/i20915/i21177).

A credit's evidence can be a registered checker that CI builds, an audit that exists but is not
registered, a C test, a cited shared tool, or prose. Those are different claims, and the ledger
should say which one each credit rests on. This tool derives the label from the tree itself, so
the table cannot drift from what is really there:

  checker    the id is in some CHECKER_ROWS that check_checker_builds discovers (the same registry
             CI uses, under tools/overlay and tests/recovered); CI proves the checker builds
  audit      the evidence document cites an existing script whose SOURCE names this body (by id,
             or by unit/segment plus offset); not registered, so NOT machine-proven to run on main
  unit-test  the document cites a `ctest -R` command for a test registered in CMakeLists.txt:
             hand-derived synthetic checks of the C, not an original-byte differential
  cites-tool the document cites only scripts that do not name this body (shared tooling)
  prose-only the document cites nothing runnable for this body
  stale      every script the document cites is missing, or a cited ctest name is not registered
  missing-doc the evidence document itself is missing

Static only: an 'audit' label means the script exists and names the body, not that it was run.

    python3 tools/overlay/credit_evidence_kind.py --write   # regenerate the committed table
    python3 tools/overlay/credit_evidence_kind.py --check   # exit 1 when the table is stale
"""
import argparse
import csv
import io
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
STRICT = "docs/re/strict_full_c_addresses.csv"
TABLE = "docs/re/credit_evidence_kind.csv"
KINDS = ("checker", "audit", "unit-test", "cites-tool", "prose-only", "stale", "missing-doc")
FIELDS = ("id", "kind", "detail")
SCRIPT = re.compile(r"(?<![\w/.-])((?:\.\./\.\./)?(?:tools/overlay|tests/recovered)/[A-Za-z0-9_.-]+\.py)")
CTEST = re.compile(r"\bctest\b[^\n`]*?\s-R\s+['\"]?\^?(\w+)\$?['\"]?")
ADD_TEST = re.compile(r"add_test\(\s*NAME\s+(\w+)")


def registered(root):
    """{credit id: [checker file names]} from check_checker_builds' own discovery, so the
    'checker' label cannot drift from the registry CI builds (#1411 scanned tools/overlay only
    and missed the tests/recovered checkers for 12B2 and 25BC)."""
    from check_checker_builds import discover
    return {rid: [path.name for path in paths] for rid, paths in discover(root).items()}


def ctest_names(root):
    cmake = root / "CMakeLists.txt"
    return set(ADD_TEST.findall(cmake.read_text())) if cmake.is_file() else set()


def _tokens(rid):
    _, area, offset = rid.split("_")
    unit = area.lstrip("0").lower() or "0"
    return area.lower(), unit, offset.lower()


def _names_in_filename(name, rid):
    area, unit, offset = _tokens(rid)
    name = name.lower()
    return bool(re.search(rf"(?<![0-9a-f]){offset}(?![0-9a-f])", name)
                and (f"u{unit}" in name or area in name or f"res{unit}" in name))


def _names_in_source(text, rid):
    area, unit, offset = _tokens(rid)
    if rid.lower() in text:
        return True
    return bool(re.search(rf"0x0*{offset.lstrip('0') or '0'}\b", text)
                and (re.search(rf"0x0*{unit}\b", text) or f"u{unit}" in text or area in text))


def classify(root=ROOT):
    """[(id, kind, detail)] for every strict credit, in the strict table's order."""
    reg = registered(root)
    known_tests = ctest_names(root)
    sources = {}

    def source(path):
        if path not in sources:
            file = root / path
            sources[path] = file.read_text(errors="ignore").lower() if file.is_file() else None
        return sources[path]

    out = []
    for row in csv.DictReader((root / STRICT).read_text().splitlines()):
        rid, doc = row["id"], row["evidence_doc"]
        if rid in reg:
            out.append((rid, "checker", ";".join(reg[rid])))
            continue
        docfile = root / doc
        if not docfile.is_file():
            out.append((rid, "missing-doc", doc))
            continue
        text = docfile.read_text(errors="ignore")
        scripts = sorted({m.replace("../../", "") for m in SCRIPT.findall(text)})
        ctests = sorted(set(CTEST.findall(text)))
        existing = [s for s in scripts if (root / s).is_file()]
        naming = [s for s in existing
                  if _names_in_filename(Path(s).name, rid) or _names_in_source(source(s), rid)]
        unknown = [name for name in ctests if name not in known_tests]
        if naming:
            out.append((rid, "audit", ";".join(naming)))
        elif (scripts and not existing) or unknown:
            out.append((rid, "stale", ";".join([s for s in scripts if s not in existing] + unknown)))
        elif ctests:
            out.append((rid, "unit-test", ";".join(ctests)))
        elif existing:
            out.append((rid, "cites-tool", ";".join(existing[:3])))
        else:
            out.append((rid, "prose-only", ""))
    return out


def render(rows):
    stream = io.StringIO(newline="")
    writer = csv.writer(stream, lineterminator="\n")
    writer.writerow(FIELDS)
    writer.writerows(rows)
    return stream.getvalue()


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true", help="regenerate the committed table")
    mode.add_argument("--check", action="store_true", help="exit 1 when the committed table is stale")
    args = parser.parse_args()
    rows = classify()
    text = render(rows)
    counts = {kind: sum(1 for _, k, _ in rows if k == kind) for kind in KINDS}
    print(f"{len(rows)} credits: " + ", ".join(f"{k} {v}" for k, v in counts.items() if v))
    path = ROOT / TABLE
    if args.write:
        path.write_text(text)
        return 0
    if not path.is_file() or path.read_text() != text:
        print(f"{TABLE} is stale: run credit_evidence_kind.py --write", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
