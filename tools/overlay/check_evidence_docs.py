#!/usr/bin/env python3
"""Fail when a credited row's evidence document does not document that row.

Every row in the accounting record names an evidence document. Every other check
we have *trusts that link*: the classifier promotes a row because the CSV says
so, `check_record_shas.py` proves the commit ids are real, and the checker gate
proves the named harness builds. A silently repointed or vanished document
survives all of them, and a future reader then "verifies" a body against the
wrong evidence.

This check closes that hole mechanically: for each record row, the named
document must exist AND mention the row — as the canonical id or as any offset
form the documents actually use (`+0x0295`, `+0295`, `0x68:0x0295`,
`RES 0464:2595`). Formatting cannot fake a pass.

LIMITATION, stated here because a checker that overstates what it proves is
worse than a missing one: a *passing reference* satisfies it, so this proves the
link resolves to the right document, NOT that the mention is substantive. It
complements the per-row doc-truth review; it does not replace it.

Scope: the three record files that carry an `evidence_doc` column. The two
ledger CSVs (`resident_callable_ledger.csv`, `overlay_function_ledger.csv`) have
no such column and are deliberately out of scope rather than exempted by list.

Exit status: 0 when every row links to a document that names it, 1 otherwise.
"""
import argparse
import csv
import re
import sys
from pathlib import Path

RECORDS = (
    "docs/re/strict_full_c_addresses.csv",
    "docs/re/reviewed_function_identities.csv",
    "docs/re/partial_callee_models.csv",
)
ROW_ID = re.compile(r"(OVL|RES)_([0-9A-F]{4})_([0-9A-F]{4})")


def accepted_forms(row_id):
    """Every textual form the evidence documents use for this row."""
    match = ROW_ID.fullmatch(row_id)
    if not match:
        return {row_id}
    kind, unit, offset = match.groups()
    forms = {
        row_id,
        f"{unit}:{offset}",
        f"0x{unit.lower()}:0x{offset.lower()}",
        f"+0x{offset}",
        f"+{offset}",
        f"+0x{offset.lower()}",
        f"+{offset.lower()}",
        f"0x{offset}",
        f"0x{offset.lower()}",
        f"{int(unit, 16)}:{offset}",
    }
    if kind == "RES":
        forms |= {f"RES {unit}:{offset}", f"RES_{unit}_{offset}"}
    return {form for form in forms if form}


def problems(root):
    """Every row whose evidence link is missing or never names the row."""
    root = Path(root)
    found = []
    for record in RECORDS:
        path = root / record
        if not path.is_file():
            found.append(f"{record}: record is missing")
            continue
        for row in csv.DictReader(path.open()):
            row_id = row.get("id")
            document = row.get("evidence_doc")
            if not row_id or not document:
                found.append(f"{record}: row without id or evidence_doc: {row}")
                continue
            target = root / document
            if not target.is_file():
                found.append(f"{record}: {row_id} names a missing document: {document}")
                continue
            text = target.read_text(errors="ignore")
            if not any(form in text for form in accepted_forms(row_id)):
                found.append(f"{record}: {row_id} is not named anywhere in {document}")
    return found


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=Path(__file__).resolve().parents[2],
                        type=Path, help="repository root")
    parser.add_argument("--quiet", action="store_true",
                        help="print only the summary line")
    args = parser.parse_args(argv)
    found = problems(args.root)
    if found and not args.quiet:
        for line in found:
            print(line)
    if found:
        print(f"EVIDENCE LINKS BROKEN: {len(found)} row(s) do not resolve to a "
              f"document that names them")
        return 1
    rows = sum(1 for record in RECORDS
               for _ in csv.DictReader((args.root / record).open()))
    print(f"OK: {rows} credited rows link to documents that name them "
          f"(link resolution only, not substantive review)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
