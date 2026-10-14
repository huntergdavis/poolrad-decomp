#!/usr/bin/env python3
"""Audit before/after strict credits, verified ownership, and classifier classes.

Uses merged #377 callable snapshots and #390 alias/manifest contracts. Supply
queue JSON and its hash-matched strict manifest for each side. No media or
disassembly is needed, and no source ledger is changed. Optional exact head
labels describe supplied snapshots; they are not independent Git validation.

Expected JSON is {"schema_version": 1, "changes": [
  {"id": "OVL_0025_0756", "field": "strict_full_c", "before": 0, "after": 1},
  {"id": "OVL_0025_0756", "field": "game_or_library",
   "before": "unknown", "after": "game_verified"},
  {"id": "OVL_0025_0756", "field": "classification",
   "before": "game_evidenced", "after": "game_verified"}
]}.

Each expectation is exact and alias-normalized. Caller class changes must be
listed too; matching a total delta does not excuse unrelated changes. Without
an expectation file, every accounting/class change is unexpected. Expectations
are comparison contracts, never approvals or automatic strict-C awards.

Usage:
  python3 tools/overlay/function_credit_delta_audit.py \
    --before-queue resources/before/queue.json \
    --after-queue resources/after/queue.json \
    --before-full-c-map resources/before/strict.csv \
    --after-full-c-map docs/re/strict_full_c_addresses.csv \
    --expected-changes resources/expected.json \
    --json resources/credit-delta.json

Exit 0: exact expected transition. Exit 1: unexpected/missing changes or
candidate/alias-set drift (report is written). Exit 2: invalid inputs (output
is not overwritten). Queue counts never become a whole-game denominator.
"""

import argparse
from collections import Counter
import csv
import hashlib
import json
from pathlib import Path
import re

from function_accounting_refresh import aliases_for
from function_queue import (OVERLAY_CLASSES, RESIDENT_CLASSES, check_inputs,
                            index_unique, ownership, require)


FIELDS = ("strict_full_c", "game_or_library", "classification")


def sha(content):
    return hashlib.sha256(content).hexdigest()


def snapshot(queue, credits):
    check_inputs(queue)
    require(queue["schema_version"] == 1, "unsupported callable queue schema")
    rows = index_unique(queue["rows"])
    aliases = aliases_for(queue)
    for row in rows.values():
        require(row["domain"] in {"overlay", "resident"}, "unknown candidate domain")
        if row["domain"] == "overlay":
            para, rel = (int(x, 16) for x in row["original_address"].split(":"))
            canonical, address = f"OVL_{para:04X}_{rel:04X}", f"{para:#x}:{rel:#06x}"
            classes = OVERLAY_CLASSES
        else:
            require(row["original_address"].startswith("image:"), "invalid resident address")
            image = int(row["original_address"].split(":")[1], 16)
            canonical, address = f"RES_IMAGE_{image:05X}", f"image:0x{image:05X}"
            classes = RESIDENT_CLASSES
        require(row["id"] == canonical and row["original_address"] == address and
                re.fullmatch(r"OVL_[0-9A-F]{4}_[0-9A-F]{4}|RES_IMAGE_[0-9A-F]{5}", canonical),
                "noncanonical original identity/address")
        require(row["classification"] in classes and
                row["game_or_library"] == ownership(row["classification"]), "inconsistent identity/class")
        require(type(row["strict_full_c"]) is int and row["strict_full_c"] in (0, 1),
                "invalid strict-credit flag")
        require(len(row["aliases"]) == len(set(row["aliases"])), "duplicate alias within original")
    credited = {}
    for credit in credits:
        require(credit["id"] in aliases, "strict manifest original absent from queue")
        target = aliases[credit["id"]]
        require(target["id"] not in credited, "duplicate strict credit through aliases")
        require(target["game_or_library"] == "game_verified", "strict original lacks verified game identity")
        doc = credit.get("evidence_doc", "")
        require(doc.startswith("docs/re/") and ".." not in doc.split("/"), "invalid strict evidence path")
        credited[target["id"]] = doc
    require(set(credited) == {key for key, row in rows.items() if row["strict_full_c"]},
            "strict manifest differs from snapshot credit flags")
    summary = queue["summary"]
    require(summary["queue_originals"] == len(rows) and summary["strict_full_c_originals"] == len(credited)
            and summary["ownership"] == dict(Counter(r["game_or_library"] for r in rows.values()))
            and summary["classifications"] == dict(Counter(r["classification"] for r in rows.values())),
            "stale callable snapshot summary")
    require(summary["whole_game_denominator"] == "unknown" and summary["completion_percentage"] is None,
            "candidate count cannot become a whole-game denominator")
    return rows, aliases, credited


def audit_delta(before, after, before_credits, after_credits, expected):
    old, old_aliases, old_credits = snapshot(before, before_credits)
    new, new_aliases, new_credits = snapshot(after, after_credits)
    require(before["inputs"] == after["inputs"], "snapshot build provenance differs")
    require(set(expected) == {"schema_version", "changes"} and expected["schema_version"] == 1
            and isinstance(expected["changes"], list), "invalid expected-change document")
    shared = set(old) & set(new)
    expectations = {}
    for change in expected["changes"]:
        require(set(change) == {"id", "field", "before", "after"}, "invalid expected transition fields")
        name, field = change["id"], change["field"]
        require(name in old_aliases and name in new_aliases, "expected original absent from either snapshot")
        key = old_aliases[name]["id"]
        require(key == new_aliases[name]["id"], "expected alias changes original identity")
        require(field in FIELDS and (key, field) not in expectations, "duplicate or unknown expected field")
        values = (change["before"], change["after"])
        if field == "strict_full_c":
            require(all(type(value) is int and value in (0, 1) for value in values), "invalid expected strict flag")
        else:
            allowed = ({"unknown", "game_verified", "library_inferred"} if field == "game_or_library"
                       else OVERLAY_CLASSES | RESIDENT_CLASSES)
            require(all(value in allowed for value in values), "invalid expected identity/class value")
        require(values[0] != values[1], "expected transition makes no change")
        expectations[key, field] = dict(change, id=key)
    actual = {(key, field): dict(id=key, field=field, before=old[key][field], after=new[key][field])
              for key in sorted(shared) for field in FIELDS if old[key][field] != new[key][field]}
    changes = [actual[key] for key in sorted(actual)]
    unexpected = [actual[key] for key in sorted(actual) if actual[key] != expectations.get(key)]
    missing = [expectations[key] for key in sorted(expectations) if expectations[key] != actual.get(key)]
    alias_changes = []
    for key in sorted(shared):
        left, right = sorted(old[key]["aliases"]), sorted(new[key]["aliases"])
        if left != right:
            alias_changes.append(dict(id=key, before_count=len(left), after_count=len(right),
                before_sha256=sha(json.dumps(left).encode()), after_sha256=sha(json.dumps(right).encode())))
    added, removed = sorted(set(new) - set(old)), sorted(set(old) - set(new))
    clean = not (unexpected or missing or added or removed or alias_changes)
    totals = lambda rows, credits: dict(queue_originals=len(rows), strict_full_c_originals=len(credits),
        verified_game_identities=sum(r["game_or_library"] == "game_verified" for r in rows.values()))
    prior, current = totals(old, old_credits), totals(new, new_credits)
    return dict(schema_version=1, inputs=before["inputs"], passed=clean,
        summary=dict(before=prior, after=current,
            delta={key: current[key] - prior[key] for key in prior},
            actual_changes=len(changes), unexpected_changes=len(unexpected), missing_expected_changes=len(missing),
            whole_game_denominator="unknown", completion_percentage=None),
        changes=changes, unexpected_changes=unexpected, missing_expected_changes=missing,
        original_set_changes=dict(added=added, removed=removed), alias_changes=alias_changes,
        strict_evidence_changes=[dict(id=key, before=old_credits[key], after=new_credits[key])
            for key in sorted(set(old_credits) & set(new_credits)) if old_credits[key] != new_credits[key]])


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    for name in ("before-queue", "after-queue", "before-full-c-map", "after-full-c-map"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--expected-changes")
    parser.add_argument("--before-head", help="optional exact commit label; not independently verified")
    parser.add_argument("--after-head", help="optional exact commit label; not independently verified")
    parser.add_argument("--json", help="output file; otherwise print report JSON")
    args = parser.parse_args()
    try:
        for head in (args.before_head, args.after_head):
            require(head is None or re.fullmatch(r"[0-9a-f]{40}", head), "head labels must be exact 40-hex commits")
        blobs = {key: Path(getattr(args, key)).read_bytes() for key in
                 ("before_queue", "after_queue", "before_full_c_map", "after_full_c_map")}
        hashes = {key: sha(value) for key, value in blobs.items()}
        queues = [json.loads(blobs[side + "_queue"]) for side in ("before", "after")]
        for side, queue in zip(("before", "after"), queues):
            require(hashes[side + "_full_c_map"] == queue["artifacts_sha256"]["full_c_map"],
                    side + " strict manifest differs from queue provenance")
        expected = dict(schema_version=1, changes=[])
        if args.expected_changes:
            blobs["expected_changes"] = Path(args.expected_changes).read_bytes()
            hashes["expected_changes"] = sha(blobs["expected_changes"])
            expected = json.loads(blobs["expected_changes"])
        credits = [list(csv.DictReader(blobs[side + "_full_c_map"].decode().splitlines()))
                   for side in ("before", "after")]
        result = audit_delta(*queues, *credits, expected)
        result["artifacts_sha256"] = hashes
        result["snapshot_head_labels"] = dict(before=args.before_head, after=args.after_head)
        content = json.dumps(result, sort_keys=True, indent=2) + "\n"
        if args.json:
            Path(args.json).write_text(content)
            print(json.dumps(dict(passed=result["passed"], **result["summary"]), sort_keys=True))
        else:
            print(content, end="")
        return 0 if result["passed"] else 1
    except (ValueError, KeyError, TypeError, OSError) as error:
        parser.exit(2, f"credit delta audit refused: {error}\n")


if __name__ == "__main__":
    raise SystemExit(main())
