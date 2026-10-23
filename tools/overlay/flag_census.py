#!/usr/bin/env python3
"""Regenerable flag-semantics census: every static helper in src/recovered that writes a FLAGS value has a row.

A helper is a `static` function whose body assigns to `flags` (`flags &=`, `flags |=`, `flags =`, also a local `flags`).
Helpers that are identical modulo struct type names, the `cpu->`/`c->`-style accessor name, comments and whitespace share one row, keyed by the md5 of the normalized body,
so a copy of an already classified helper needs no new row and a CHANGED body needs a fresh verdict.

  flag_census.py --check   exit 1 if a helper has no row or its row is UNCLASSIFIED (the CI-able gate)
  flag_census.py --carry-from REV   after editing helper bodies, keep each row on its edited helper (matched by file and
                           function against REV's tree) and mark disagree rows FIXED; then run --regen
  flag_census.py --regen   refresh the files/n_helpers columns, drop rows no helper matches, add UNCLASSIFIED rows

The classified_by column says who read the helper against the pinned source ("agent only" = a read-only sub-agent; "agent + body
hand-read when fixed" = the body was also read and edited by hand). The verdict columns are written by a person after reading the PINNED DOSBox-X source (stored FillFlags rule; see
docs/re/FLAG_SEMANTICS_CENSUS.md). The branched_on column needs the original bytes (resources/) and is kept as data.
"""
import argparse
import csv
import hashlib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TSV = ROOT / "docs/re/FLAG_SEMANTICS_CENSUS.tsv"
COLUMNS = ["hash", "n_helpers", "files", "instruction", "assumed_rule", "pinned_rule_and_citation", "agent_verdict",
           "stored_flags_verdict(FillFlags)", "detail", "classified_by", "branched_on"]
FUNCTION = re.compile(r"^static\s+[A-Za-z0-9_ \*]+?\b([A-Za-z0-9_]+)\s*\(([^)]*)\)\s*\{", re.M)
WRITES_FLAGS = re.compile(r"\bflags\s*(?:&=|\|=|\^=|=(?!=))")
UNCLASSIFIED = "UNCLASSIFIED"


def normalize(body):
    n = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    n = re.sub(r"//.*", "", n)
    n = re.sub(r"\b(cpu|c|st|state|m|h)->", "X->", n)
    n = re.sub(r"pr_[a-z0-9_]+|cpu_t|machine", "T", n)
    n = re.sub(r"^static [^\(]*\(", "static F(", n)
    return re.sub(r"\s+", "", n)


def helpers(root=ROOT):
    """Yield (file name, function name, body, hash) for every flag-writing static function."""
    for path in sorted((Path(root) / "src/recovered").glob("*.c")):
        text = path.read_text()
        for m in FUNCTION.finditer(text):
            i, depth = m.end(), 1
            while depth and i < len(text):
                depth += (text[i] == "{") - (text[i] == "}")
                i += 1
            body = text[m.start():i]
            if WRITES_FLAGS.search(body):
                yield path.name, m.group(1), body, hashlib.md5(normalize(body).encode()).hexdigest()


def groups(root=ROOT):
    found = {}
    for name, function, _, digest in helpers(root):
        found.setdefault(digest, []).append((name, function))
    return found


def read_rows(path=TSV):
    with open(path, newline="") as handle:
        return {row["hash"]: row for row in csv.DictReader(handle, delimiter="\t")}


def write_rows(rows, path=TSV):
    with open(path, "w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=COLUMNS, delimiter="\t", lineterminator="\n", restval="")
        writer.writeheader()
        for digest in sorted(rows, key=lambda d: (rows[d]["files"], d)):
            writer.writerow(rows[digest])


def gaps(root=ROOT, path=TSV):
    """(helpers with no row, helpers whose row is UNCLASSIFIED)."""
    rows, found = read_rows(path), groups(root)
    missing = sorted((f, fn) for d, members in found.items() if d not in rows for f, fn in members)
    pending = sorted((f, fn) for d, members in found.items() if d in rows and rows[d]["agent_verdict"] == UNCLASSIFIED
                     for f, fn in members)
    return missing, pending


def regen(root=ROOT, path=TSV):
    rows, found = (read_rows(path) if Path(path).exists() else {}), groups(root)
    out, dropped = {}, 0
    for digest, members in found.items():
        row = dict(rows.get(digest) or {c: "" for c in COLUMNS})
        if not row["agent_verdict"]:
            row.update(agent_verdict=UNCLASSIFIED, **{"stored_flags_verdict(FillFlags)": UNCLASSIFIED,
                                                       "branched_on": "not scanned (new row)"})
        row["classified_by"] = row.get("classified_by") or "agent only"
        row.update(hash=digest, n_helpers=str(len(members)), files=" ".join(sorted({f for f, _ in members})))
        if not row["instruction"]:
            row["instruction"] = ", ".join(sorted({fn for _, fn in members}))[:80]
        out[digest] = row
    dropped = len(set(rows) - set(out))
    write_rows(out, path)
    return len(out), dropped, sum(1 for r in out.values() if r["agent_verdict"] == UNCLASSIFIED)


def carry_from(rev, root=ROOT, path=TSV):
    """Move rows whose helper body changed since `rev` onto the new hash (matched by file and function)."""
    import subprocess
    import tarfile
    import io
    import tempfile
    blob = subprocess.run(["git", "-C", str(root), "archive", rev, "src/recovered"], check=True, capture_output=True).stdout
    with tempfile.TemporaryDirectory() as tmp:
        tarfile.open(fileobj=io.BytesIO(blob)).extractall(tmp)
        old = {(f, fn): d for f, fn, _, d in helpers(Path(tmp))}
    new = {(f, fn): d for f, fn, _, d in helpers(root)}
    rows, moved = read_rows(path), 0
    for key, od in old.items():
        nd = new.get(key)
        if not nd or nd == od or od not in rows:
            continue
        row = dict(rows[od])
        row["hash"] = nd
        if row["stored_flags_verdict(FillFlags)"].startswith("disagree"):
            row["stored_flags_verdict(FillFlags)"] = "FIXED to pinned stored flags"
        rows.setdefault(nd, row)
        moved += 1
    for od in {old[k] for k in old if new.get(k) not in (None, old[k])}:
        if od in rows and od not in set(new.values()):
            del rows[od]
    write_rows(rows, path)
    return moved


def main(argv=None):
    p = argparse.ArgumentParser()
    p.add_argument("--check", action="store_true")
    p.add_argument("--regen", action="store_true")
    p.add_argument("--carry-from", metavar="REV")
    a = p.parse_args(argv)
    if a.carry_from:
        print(f"{carry_from(a.carry_from)} rows carried from {a.carry_from}")
    if a.regen:
        rows, dropped, pending = regen()
        print(f"{rows} rows written, {dropped} stale rows dropped, {pending} UNCLASSIFIED")
    missing, pending = gaps()
    for f, fn in missing:
        print(f"NO ROW: {f}:{fn}")
    for f, fn in pending:
        print(f"UNCLASSIFIED: {f}:{fn}")
    if a.check or a.regen:
        print(f"census gaps: {len(missing)} without a row, {len(pending)} unclassified")
    return 1 if (a.check and (missing or pending)) else 0


if __name__ == "__main__":
    sys.exit(main())
