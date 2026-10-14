#!/usr/bin/env python3
"""Regenerate the census from hash-pinned local media; emit counts/hashes only.

Requires Capstone 5.0.7. Products stay in the chosen local output directory;
the input media are read without modification. No recovered C is executed.
"""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

from function_queue import DIGESTS, DEFAULT_CREDIT
from classify_overlay import DEFAULT_REVIEWED_IDENTITIES


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--ovr", required=True)
    parser.add_argument("--out-dir", required=True)
    parser.add_argument("--full-c-map", default=str(DEFAULT_CREDIT))
    parser.add_argument("--reviewed-identities", default=str(DEFAULT_REVIEWED_IDENTITIES))
    parser.add_argument("--refresh-classification", action="store_true",
                        help="stage a newly reviewed classifier; keep other ledger comparisons strict")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    exe, ovr = Path(args.exe).resolve(), Path(args.ovr).resolve()
    out = Path(args.out_dir).resolve()
    for path, key in ((exe, "start_exe_sha256"), (ovr, "game_ovr_sha256")):
        if hashlib.sha256(path.read_bytes()).hexdigest() != DIGESTS[key]:
            parser.exit(2, "owner-media hash mismatch; no audit products written\n")
    import capstone
    if capstone.__version__ != "5.0.7":
        parser.exit(2, "reproducible audit requires Capstone 5.0.7\n")
    out.mkdir(parents=True, exist_ok=True)
    def p(name):
        return str(out / name)
    def run(script, *arguments):
        result = subprocess.run([sys.executable, str(root / "tools/overlay" / script),
                                 *map(str, arguments)], cwd=root, capture_output=True, text=True)
        # These existing tools emit address/count metadata. Keep diagnostics local.
        (out / (script + ".log")).write_text(result.stdout + result.stderr)
        if result.returncode:
            parser.exit(2, f"audit stage failed: {script}; inspect local stage log\n")
    run("overlay_inventory.py", "--exe", exe, "--ovr", ovr, "--json", p("inventory.json"))
    run("callgraph.py", "--inventory", p("inventory.json"), "--exe", exe, "--ovr", ovr,
        "--json", p("callgraph.json"))
    run("validate_near_calls.py", "--inventory", p("inventory.json"), "--callgraph", p("callgraph.json"),
        "--ovr", ovr, "--json", p("near.json"))
    run("function_ledger.py", "--inventory", p("inventory.json"), "--callgraph", p("callgraph.json"),
        "--near-calls", p("near.json"), "--exe", exe, "--ovr", ovr,
        "--csv", p("overlay.csv"), "--md", p("overlay.md"))
    run("classify_overlay.py", "--inventory", p("inventory.json"), "--callgraph", p("callgraph.json"),
        "--ledger-csv", p("overlay.csv"), "--ovr", ovr,
        "--reviewed-identities", Path(args.reviewed_identities).resolve(),
        "--csv", p("classification.csv"), "--md", p("classification.md"))
    run("resident_callable_coverage.py", "--exe", exe, "--ovr", ovr,
        "--json", p("resident.json"), "--csv", p("resident.csv"))
    for generated, checked_in in (("overlay.csv", "overlay_function_ledger.csv"),
                                   ("classification.csv", "overlay_classification.csv"),
                                   ("resident.csv", "resident_callable_ledger.csv")):
        if generated == "classification.csv" and args.refresh_classification:
            continue
        if (out / generated).read_bytes() != (root / "docs/re" / checked_in).read_bytes():
            parser.exit(2, f"regenerated ledger differs: {checked_in}\n")
    for filename in ("queue.json", "queue-repeat.json"):
        run("function_queue.py", "--inventory", p("inventory.json"), "--ledger", p("overlay.csv"),
            "--classification", p("classification.csv"), "--near-calls", p("near.json"),
            "--full-c-map", Path(args.full_c_map).resolve(),
            "--reviewed-identities", Path(args.reviewed_identities).resolve(),
            "--resident-audit", p("resident.json"), "--json", p(filename))
    content = (out / "queue.json").read_bytes()
    if content != (out / "queue-repeat.json").read_bytes():
        parser.exit(2, "queue exports differ between identical runs\n")
    result = json.loads(content)
    print(json.dumps(dict(inputs=DIGESTS, summary=result["summary"],
                          queue_sha256=hashlib.sha256(content).hexdigest()), sort_keys=True, indent=2))


if __name__ == "__main__":
    main()
