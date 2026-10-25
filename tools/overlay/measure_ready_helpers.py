#!/usr/bin/env python3
"""Paired ready-helper measurements; run through poolrad-heavy.sh.

Exports contain addresses, counts and hashes only. The 16EC diagnostic is an
explicit hypothetical closure, not an admission, credit or replacement grant.
"""
import argparse
import contextlib
import csv
import io
import json
from pathlib import Path
import subprocess
import sys

import strict_candidate_queue as queue


def differences(before, after):
    previous = {row["id"]: row["callee_readiness"] for row in before["rows"]}
    return [dict(id=row["id"], before=previous[row["id"]], after=row["callee_readiness"])
            for row in after["rows"] if previous[row["id"]] != row["callee_readiness"]]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--expected-base", required=True)
    parser.add_argument("--inputs", type=Path, default=queue.ROOT / "resources/function-accounting-current")
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    root = queue.ROOT
    commit = subprocess.check_output(["git", "-C", str(root), "rev-parse", "HEAD"], text=True).strip()
    base = subprocess.check_output(["git", "-C", str(root), "rev-parse", args.expected_base], text=True).strip()
    subprocess.run(["git", "-C", str(root), "merge-base", "--is-ancestor", base, commit], check=True)
    for name, relative in queue.FRESHNESS_INPUTS.items():
        if name.startswith("ready_helper_"):
            continue  # This task's rule/evidence changes are the treatment.
        original = subprocess.check_output(["git", "-C", str(root), "show", f"{base}:{relative}"])
        queue.require(original == (root / relative).read_bytes(), f"{relative} differs from measurement base {base}")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    credit_paths = [root / "docs/re/strict_full_c_addresses.csv", root / "docs/re/replaced_bodies.json"]
    credit_bytes = [path.read_bytes() for path in credit_paths]
    credited = len(list(csv.DictReader(io.StringIO(credit_bytes[0].decode()))))
    replaced = len(json.loads(credit_bytes[1])["bodies"])

    def export(name, *, without=False, omit=None, hypothetical_16ec=False):
        path = args.output_dir / f"{name}.json"
        argv = ["strict_candidate_queue.py", "--inventory", str(args.inputs / "inventory.json"),
                "--callgraph", str(args.inputs / "callgraph.json"), "--near-calls", str(args.inputs / "near.json"),
                "--ovr", str(root / "resources/DOS/POOLRAD/GAME.OVR"), "--repo", str(root),
                "--no-git", "--json", str(path)]
        if without:
            argv.append("--without-ready-helpers")
        saved_argv, saved_load, saved_members = sys.argv, queue.load_queue, queue.READY_HELPER_MEMBERS

        def hypothetical(*a, **kw):
            state = saved_load(*a, **kw)
            state["resident_full"].add("RES_00BA_16EC")
            return state

        try:
            sys.argv = argv
            if omit:
                queue.READY_HELPER_MEMBERS = saved_members - {omit}
            if hypothetical_16ec:
                queue.load_queue = hypothetical
            with contextlib.redirect_stdout(io.StringIO()):
                queue.main()
        finally:
            sys.argv, queue.load_queue, queue.READY_HELPER_MEMBERS = saved_argv, saved_load, saved_members
        result = json.loads(path.read_text())
        if hypothetical_16ec:
            result["diagnostic_only"] = "ASSUMED RES_00BA_16EC manifest-full; not actual admission or credit"
            path.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
        return result

    def totals(result):
        return dict(candidates=result["summary"]["candidates"],
                    composable=result["summary"]["composable_bodies"],
                    members=result["ready_helpers"]["members"], credited=credited, replaced=replaced)

    without = export("without")
    with_rule = export("with")
    queue.require(without["artifacts_sha256"] == with_rule["artifacts_sha256"], "paired input hashes differ")
    pairs = []
    for member in sorted(queue.READY_HELPER_MEMBERS):
        omitted = export(f"without-{member}", omit=member)
        pairs.append(dict(member=member, without=totals(omitted), with_rule=totals(with_rule),
                          changed_rows=differences(omitted, with_rule)))
    diagnostic = export("diagnostic-16ec-covered", hypothetical_16ec=True)
    newly_composable = sorted(row["id"] for row in differences(with_rule, diagnostic)
                              if not row["before"]["composable"] and row["after"]["composable"])
    handlers = {row["handler_id"] for row in json.loads((root / "docs/re/u9e_dispatch_handlers.json").read_text())["slots"]}
    report = dict(base=base, measured_head=commit, scan="--no-git, same tree and hash-pinned inputs",
                  artifacts_sha256=with_rule["artifacts_sha256"], without=totals(without),
                  with_rule=totals(with_rule), changed_rows=differences(without, with_rule),
                  per_member=pairs, candidates=with_rule["ready_helpers"]["candidates"],
                  diagnostic_16ec=dict(assumption="16EC full closure ready (not established by t81)",
                                       totals=totals(diagnostic), newly_composable=newly_composable,
                                       handler_closures=sorted(set(newly_composable) & handlers)))
    queue.require(credit_bytes == [path.read_bytes() for path in credit_paths], "credit tables changed during measurement")
    (args.output_dir / "measurement.json").write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(json.dumps(report, indent=2, sort_keys=True))
    for pair in pairs:
        if not pair["changed_rows"]:
            print(f"NO EFFECT: {pair['member']} changes no candidate readiness row.")
    if report["without"] == report["with_rule"]:
        print("NO EFFECT: WITH and WITHOUT totals are identical. No member has been admitted.")


if __name__ == "__main__":
    main()
