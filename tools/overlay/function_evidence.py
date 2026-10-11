#!/usr/bin/env python3
"""Count overlay function-start evidence without treating patterns as functions.

Consumes the private JSON products of overlay_inventory.py and callgraph.py.
The report contains only addresses, hashes, and aggregate counts. Near-call
evidence remains provisional because callgraph.py scans bytes, not decoded
instruction boundaries.
"""

import argparse
import json
from pathlib import Path


def report(inventory, callgraph):
    units = inventory["units"]
    prologues = {(u["stub_para"], f["rel"]) for u in units for f in u["functions"]}
    entries = {(u["stub_para"], rel) for u in units for rel in u["entries"]}
    far_targets = {
        (edge["target_unit"], edge["target_rel"])
        for edge in callgraph["edges"] if edge.get("resolution") == "overlay"
    }
    near_candidates = {
        (edge["unit"], edge["target"])
        for edge in callgraph["edges"]
        if edge["kind"] == "near_candidate" and edge.get("exact_prologue")
    }
    anomalous = []
    for unit in units:
        starts = {f["rel"] for f in unit["functions"]}
        for index, target in enumerate(unit["entries"]):
            if target in starts:
                continue
            anomalous.append({
                "unit": unit["stub_para"], "entry": index, "target": target,
                "code_size": unit["code_size"],
                "in_code": target < unit["code_size"],
                "fixup_confirmed_far_target": (unit["stub_para"], target) in far_targets,
            })
    return {
        "inputs": inventory["inputs"],
        "overlay_units": len(units),
        "overlay_code_bytes": inventory["coverage"]["code_total"],
        "overlay_file_gaps": len(inventory["coverage"]["gaps"]),
        "public_entry_slots": sum(len(u["entries"]) for u in units),
        "unique_public_entry_targets": len(entries),
        "prologue_candidates_in_code": len(prologues),
        "prologue_hits_anywhere_in_file": inventory["total_prologues_in_ovr"],
        "public_entries_on_prologues": sum(
            (u["stub_para"], target) in prologues
            for u in units for target in u["entries"]
        ),
        "prologues_with_public_entry": len(prologues & entries),
        "prologues_with_fixup_confirmed_far_target": len(prologues & far_targets),
        "prologues_with_entry_or_far_target": len(prologues & (entries | far_targets)),
        "prologues_with_raw_near_call_candidate": len(prologues & near_candidates),
        "prologues_with_any_listed_evidence": len(
            prologues & (entries | far_targets | near_candidates)
        ),
        "prologues_without_listed_evidence": [
            {"unit": unit, "offset": offset}
            for unit, offset in sorted(prologues - (entries | far_targets | near_candidates))
        ],
        "nonprologue_public_entries": anomalous,
        "distinct_far_targets_outside_prologues": len(far_targets - prologues),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inventory", required=True)
    parser.add_argument("--callgraph", required=True)
    parser.add_argument("--json", help="write counts and offsets to this file")
    args = parser.parse_args()
    inventory = json.loads(Path(args.inventory).read_text())
    callgraph = json.loads(Path(args.callgraph).read_text())
    result = report(inventory, callgraph)
    output = json.dumps(result, indent=2) + "\n"
    if args.json:
        Path(args.json).write_text(output)
    else:
        print(output, end="")


if __name__ == "__main__":
    main()
