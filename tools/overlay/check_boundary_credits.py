#!/usr/bin/env python3
"""Fail when a declared boundary endpoint appears in a credit table.

`strict_candidate_queue.require_not_declared_boundary` protects the QUEUE — the
place the class is *consumed*. Nothing protected the credited CSVs themselves,
so a hand edit or a bad merge could add a declared boundary endpoint to
`strict_full_c_addresses.csv` and every gate would stay green: the evidence-link
check would happily count the row, and the checker gate only looks at registered
rows. This is the missing half: the guard where the damage would happen.

A declared boundary endpoint is a dependency that may be COMPOSED. It is never a
credit. Exit status: 0 when neither credit table names one, 1 otherwise.
"""
import argparse
import csv
import sys
from pathlib import Path

from strict_candidate_queue import DECLARED_BOUNDARY_ENDPOINTS

ROOT = Path(__file__).resolve().parents[2]
CREDIT_TABLES = (
    "docs/re/strict_full_c_addresses.csv",
    "docs/re/reviewed_function_identities.csv",
)


def offences(root=ROOT):
    """Every (table, row id) where a credit table names a declared boundary."""
    found = []
    for table in CREDIT_TABLES:
        path = Path(root) / table
        if not path.is_file():
            found.append((table, "<missing table>"))
            continue
        with path.open() as handle:
            for row in csv.DictReader(handle):
                row_id = (row.get("id") or "").strip()
                if row_id in DECLARED_BOUNDARY_ENDPOINTS:
                    found.append((table, row_id))
    return found


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    args = parser.parse_args(argv)
    found = offences(args.root)
    if found:
        for table, row_id in found:
            print(f"DECLARED BOUNDARY CREDITED: {row_id} appears in {table}")
        print(f"REFUSED: {len(found)} row(s) - a declared boundary endpoint is "
              f"COMPOSABLE, never a credit (class: {sorted(DECLARED_BOUNDARY_ENDPOINTS)[0]}...)")
        return 1
    print(f"OK: no declared boundary endpoint in the {len(CREDIT_TABLES)} credit tables "
          f"({len(DECLARED_BOUNDARY_ENDPOINTS)} endpoints guarded)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
